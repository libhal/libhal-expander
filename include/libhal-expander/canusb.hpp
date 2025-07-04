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

#pragma once

#include <libhal/can.hpp>
#include <libhal/circular_buffer.hpp>
#include <libhal/pointers.hpp>
#include <libhal/serial.hpp>
#include <libhal/units.hpp>
#include <memory_resource>

namespace hal::expander {
/**
 *
 */
class canusb : public hal::v5::enable_strong_from_this<canusb>
{
public:
  static hal::v5::strong_ptr<canusb> create(
    std::pmr::polymorphic_allocator<> p_allocator,
    hal::v5::strong_ptr<hal::v5::serial> const& p_serial);

  canusb(hal::v5::strong_ptr_only_token,
         hal::v5::strong_ptr<hal::v5::serial> const& p_serial);

private:
  friend hal::v5::strong_ptr<hal::v5::can_bus_manager> acquire_can_bus_manager(
    std::pmr::polymorphic_allocator<> p_allocator,
    hal::v5::strong_ptr<canusb> const& p_manager);

  friend hal::v5::strong_ptr<hal::can_transceiver> acquire_can_transceiver(
    std::pmr::polymorphic_allocator<> p_allocator,
    hal::v5::strong_ptr<canusb> const& p_manager,
    hal::usize p_buffer_size);

  friend class canusb_bus_manager;
  friend class canusb_transceiver;

  hal::v5::strong_ptr<hal::v5::serial> m_serial;
  bool m_bus_manager_acquired = false;
  bool m_transceiver_acquired = false;
  bool m_is_open = false;
  hal::u32 m_current_baud_rate = 125000;  // Default to 125kHz
};

hal::v5::strong_ptr<hal::v5::can_bus_manager> acquire_can_bus_manager(
  std::pmr::polymorphic_allocator<> p_allocator,
  hal::v5::strong_ptr<canusb> const& p_manager);

hal::v5::strong_ptr<hal::can_transceiver> acquire_can_transceiver(
  std::pmr::polymorphic_allocator<> p_allocator,
  hal::v5::strong_ptr<canusb> const& p_manager,
  hal::usize p_buffer_size);
}  // namespace hal::expander

namespace hal {
using hal::expander::acquire_can_bus_manager;
using hal::expander::acquire_can_transceiver;
}  // namespace hal
