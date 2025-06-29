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
#include <optional>

#include <libhal/circular_buffer.hpp>
#include <libhal/pointers.hpp>
#include <libhal/serial.hpp>
#include <libhal/units.hpp>

namespace hal::expander {
/**
 *
 */
class canusb
{
public:
  canusb(hal::v5::strong_ptr<hal::v5::serial>& p_serial)
    : m_serial(std::move(p_serial))
  {
  }

  class can_bus_manager : public hal::v5::can_bus_manager
  {
    can_bus_manager(hal::v5::strong_ptr<canusb>& p_manager)
      : m_manager(std::move(p_manager))
    {
    }
    void driver_baud_rate(hal::u32 p_hertz) override;
    void driver_filter_mode(accept p_accept) override;
    void driver_on_bus_off(optional_bus_off_handler& p_callback) override;
    void driver_bus_on() override;
    hal::v5::strong_ptr<canusb> m_manager;
  };

  hal::v5::strong_ptr<can_bus_manager> acquire_can_bus_manager(
    std::pmr::polymorphic_allocator<> p_allocator);

  class can_transceiver : public hal::v5::can_transceiver
  {
    can_transceiver(hal::v5::strong_ptr<canusb>& p_manager,
                    std::pmr::polymorphic_allocator<> p_allocator,
                    hal::usize p_capacity)
      : m_manager(std::move(p_manager))
      , m_circular_buffer(p_allocator, p_capacity)
    {
    }
    u32 driver_baud_rate() override;
    void driver_send(hal::v5::can_message const& p_message) override;
    std::span<hal::v5::can_message const> driver_receive_buffer() override;
    std::size_t driver_receive_cursor() override;
    hal::v5::strong_ptr<canusb> m_manager;
    hal::v5::circular_buffer<hal::v5::can_message> m_circular_buffer;
  };

  hal::v5::strong_ptr<can_bus_manager> acquire_can_transceiver(
    std::pmr::polymorphic_allocator<> p_allocator,
    hal::usize p_buffer_size);

private:
  hal::v5::strong_ptr<hal::v5::serial> m_serial;
};
}  // namespace hal::expander
