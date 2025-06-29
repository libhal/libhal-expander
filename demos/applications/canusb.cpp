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

#include <cinttypes>

#include <libhal-util/as_bytes.hpp>
#include <memory_resource>

#include <libhal-expander/canusb.hpp>
#include <libhal-util/serial.hpp>
#include <libhal-util/steady_clock.hpp>
#include <libhal/can.hpp>
#include <libhal/pointers.hpp>

#include <resource_list.hpp>

namespace hal {

/**
 * @ingroup Serial
 * @brief Write formatted string data to serial buffer and drop return value
 *
 * Uses snprintf internally and writes to a local statically allocated an array.
 * This function will never dynamically allocate like how standard std::printf
 * does.
 *
 * This function does NOT include the NULL character when transmitting the data
 * over the serial port.
 *
 * @tparam buffer_size - Size of the buffer to allocate on the stack to store
 * the formatted message.
 * @tparam Parameters - printf arguments
 * @param p_serial - serial port to write data to
 * @param p_format - printf style null terminated format string
 * @param p_parameters - printf arguments
 */
template<size_t buffer_size, typename... Parameters>
void print(v5::serial& p_serial,
           char const* p_format,
           Parameters... p_parameters)
{
  static_assert(buffer_size > 2);
  constexpr int unterminated_max_string_size =
    static_cast<int>(buffer_size) - 1;

  std::array<char, buffer_size> buffer{};
  int length =
    std::snprintf(buffer.data(), buffer.size(), p_format, p_parameters...);

  if (length > unterminated_max_string_size) {
    // Print out what was able to be written to the buffer
    length = unterminated_max_string_size;
  }

  p_serial.write(as_bytes(std::string_view(buffer.data(), length)));
}
std::uint64_t future_deadline(hal::v5::steady_clock& p_steady_clock,
                              hal::time_duration p_duration)
{
  using period = decltype(p_duration)::period;
  auto const frequency = p_steady_clock.frequency();
  auto const tick_period = wavelength<period>(static_cast<float>(frequency));
  auto ticks_required = p_duration / tick_period;
  using unsigned_ticks = std::make_unsigned_t<decltype(ticks_required)>;

  if (ticks_required <= 1) {
    ticks_required = 1;
  }

  auto const ticks = static_cast<unsigned_ticks>(ticks_required);
  auto const future_timestamp = ticks + p_steady_clock.uptime();

  return future_timestamp;
}
void delay(hal::v5::steady_clock& p_steady_clock, hal::time_duration p_duration)
{
  auto ticks_until_timeout = future_deadline(p_steady_clock, p_duration);
  while (p_steady_clock.uptime() < ticks_until_timeout) {
    continue;
  }
}
}  // namespace hal

void application(resource_list&)
{
  using namespace std::literals;
  using namespace hal::literals;

  auto serial = usb_serial();
  auto canusb = hal::v5::make_strong_ptr<hal::expander::canusb>(
    std::pmr::new_delete_resource(), serial);

  auto manager =
    canusb->acquire_can_bus_manager(std::pmr::new_delete_resource());
  manager->baud_rate(1_MHz);
  manager->filter_mode(hal::v5::can_bus_manager::accept::all);
  manager->bus_on();
  // manager->on_bus_off()

  auto transceiver =
    canusb->acquire_can_transceiver(std::pmr::new_delete_resource(), 32);

  auto clock = steady_clock();
  auto console = serial_console(512);
  console->write(hal::as_bytes("[canusb] Application Starting...\n\n"sv));

  auto const receive_buffer = transceiver->receive_buffer();
  auto previous_cursor = transceiver->receive_cursor();

  while (true) {
    auto const cursor = transceiver->receive_cursor();

    if (cursor == previous_cursor) {
      continue;
    }

    console->write(hal::as_bytes("Received: \n"sv));

    if (cursor < previous_cursor) {
      for (auto const& message : receive_buffer.subspan(previous_cursor)) {
        hal::print<32>(*console, "  id: 0x%08" PRIX32 "\n", message.id);
      }
      previous_cursor = 0;
    }

    auto const delta = cursor - previous_cursor;
    for (auto const& message : receive_buffer.subspan(previous_cursor, delta)) {
      hal::print<32>(*console, "  id: 0x%08" PRIX32 "\n", message.id);
    }

    previous_cursor = cursor;

    hal::delay(*clock, 1s);
  }
}
