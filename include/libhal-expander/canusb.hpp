// Copyright 2024 - 2025 Khalil Estell and the libhal contributors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * @file canusb.hpp
 * @brief Driver for Lawicel CANUSB-compatible USB-to-CAN adapters
 *
 * This file provides a driver implementation for USB-to-CAN adapters that
 * use the Lawicel CANUSB protocol. The driver converts a serial port interface
 * (connected to the USB-to-CAN hardware) into CAN bus functionality.
 */

#pragma once

#include <libhal/can.hpp>
#include <libhal/circular_buffer.hpp>
#include <libhal/pointers.hpp>
#include <libhal/serial.hpp>
#include <libhal/units.hpp>
#include <memory_resource>

namespace hal::expander {

/**
 * @brief Driver for Lawicel CANUSB-compatible USB-to-CAN adapters
 *
 * The canusb driver enables libhal applications to communicate over CAN bus
 * through USB-to-CAN adapter hardware. It takes a serial port interface that
 * is physically connected to a CANUSB-compatible device and provides:
 *
 * - CAN transceiver functionality for sending/receiving CAN messages
 * - CAN bus manager for configuring bus parameters and managing resources
 *
 * The driver implements the Lawicel CANUSB protocol, which uses ASCII commands
 * sent over the serial interface to control the CAN adapter.
 *
 * Example usage:
 * @code
 * // Create the driver with a serial port connected to USB-to-CAN hardware
 * auto canusb_driver = hal::expander::canusb::create(allocator, serial_port);
 *
 * // Acquire CAN resources
 * auto bus_manager = hal::acquire_can_bus_manager(allocator, canusb_driver);
 * auto transceiver = hal::acquire_can_transceiver(allocator, canusb_driver,
 * 64);
 *
 * // Configure and use CAN bus
 * bus_manager->configure({.baud_rate = 500_kHz});
 * // ... use transceiver for CAN communication
 * @endcode
 *
 * @note Only one CAN bus manager and one CAN transceiver can be acquired
 *       from a single canusb instance at a time.
 * @note Even though the protocol is called canusb, it can work over UART and
 *       RS232.
 */
class canusb : public hal::v5::enable_strong_from_this<canusb>
{
public:
  /**
   * @brief Factory method to create a new canusb driver instance
   *
   * Creates and initializes a new CANUSB driver instance that will communicate
   * with the USB-to-CAN adapter through the provided serial port.
   *
   * @param p_allocator Memory allocator for creating the driver instance
   * @param p_serial Serial port interface connected to the USB-to-CAN hardware.
   *                 This should be configured with appropriate baud rate
   *                 (typically 115200) and other settings for the specific
   *                 USB-to-CAN adapter being used.
   *
   * @return Strong pointer to the created canusb driver instance
   *
   * @throws hal::io_error If communication with the USB-to-CAN adapter fails
   *                       during initialization
   * @throws std::bad_alloc If memory allocation fails
   */
  static hal::v5::strong_ptr<canusb> create(
    std::pmr::polymorphic_allocator<> p_allocator,
    hal::v5::strong_ptr<hal::v5::serial> const& p_serial);

  /**
   * @brief Constructor for canusb driver
   *
   * @warning This constructor should not be called directly. Use the static
   *          create() method instead to ensure proper initialization.
   *
   * @param p_serial Serial port interface connected to the USB-to-CAN hardware
   */
  canusb(hal::v5::strong_ptr_only_token,
         hal::v5::strong_ptr<hal::v5::serial> const& p_serial);

private:
  // Friend declarations for resource acquisition functions
  friend hal::v5::strong_ptr<hal::v5::can_bus_manager> acquire_can_bus_manager(
    std::pmr::polymorphic_allocator<> p_allocator,
    hal::v5::strong_ptr<canusb> const& p_manager);

  friend hal::v5::strong_ptr<hal::can_transceiver> acquire_can_transceiver(
    std::pmr::polymorphic_allocator<> p_allocator,
    hal::v5::strong_ptr<canusb> const& p_manager,
    hal::usize p_buffer_size);

  // Friend declarations for implementation classes
  friend class canusb_bus_manager;
  friend class canusb_transceiver;

  /// Serial port interface for communicating with the USB-to-CAN adapter
  hal::v5::strong_ptr<hal::v5::serial> m_serial;

  /// Flag indicating if the CAN bus manager has been acquired
  bool m_bus_manager_acquired = false;

  /// Flag indicating if the CAN transceiver has been acquired
  bool m_transceiver_acquired = false;

  /// Flag indicating if the CAN channel is currently open
  bool m_is_open = false;

  /// Current configured baud rate in Hz (default: 125kHz)
  hal::u32 m_current_baud_rate = 125000;
};

/**
 * @brief Acquire a CAN bus manager from a canusb driver
 *
 * Creates a CAN bus manager that can configure the CAN bus parameters and
 * manage the overall bus state through the USB-to-CAN adapter.
 *
 * @param p_allocator Memory allocator for creating the bus manager
 * @param p_manager The canusb driver instance to acquire the manager from
 *
 * @return Strong pointer to the CAN bus manager interface
 *
 * @throws hal::device_or_resource_busy If a bus manager has already been
 *                                      acquired from this canusb instance
 * @throws std::bad_alloc If memory allocation fails
 *
 * @note Only one bus manager can be acquired per canusb instance. The bus
 *       manager must be destroyed before another can be acquired.
 * @note APIs `on_bus_off` and `filter_mode` both do nothing. The default filter
 *       mode is "accept::all". The bus off event will not be called if the
 *       device goes into bus off state. Do not use this for serious projects
 *.      until this notice is removed.
 */
hal::v5::strong_ptr<hal::v5::can_bus_manager> acquire_can_bus_manager(
  std::pmr::polymorphic_allocator<> p_allocator,
  hal::v5::strong_ptr<canusb> const& p_manager);

/**
 * @brief Acquire a CAN transceiver from a canusb driver
 *
 * Creates a CAN transceiver that can send and receive CAN messages through
 * the USB-to-CAN adapter. The transceiver includes internal buffering for
 * received messages.
 *
 * @param p_allocator Memory allocator for creating the transceiver
 * @param p_manager The canusb driver instance to acquire the transceiver from
 * @param p_buffer_size Size of the internal receive buffer for CAN messages.
 *                      Larger buffers can handle bursts of incoming messages
 *                      better but consume more memory. If passed 0, will
 *                      become 1.
 *
 * @return Strong pointer to the CAN transceiver interface
 *
 * @throws hal::device_or_resource_busy If a transceiver has already been
 *                                      acquired from this canusb instance
 * @throws std::bad_alloc If memory allocation fails
 *
 * @note Only one transceiver can be acquired per canusb instance. The
 *       transceiver must be destroyed before another can be acquired.
 */
hal::v5::strong_ptr<hal::can_transceiver> acquire_can_transceiver(
  std::pmr::polymorphic_allocator<> p_allocator,
  hal::v5::strong_ptr<canusb> const& p_manager,
  hal::usize p_buffer_size);

}  // namespace hal::expander

namespace hal {
// Import acquisition functions into the hal namespace for convenience
using hal::expander::acquire_can_bus_manager;
using hal::expander::acquire_can_transceiver;
}  // namespace hal
