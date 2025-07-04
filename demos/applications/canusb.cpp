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

void application()
{
  using namespace std::literals;
  using namespace hal::literals;

  hal::v5::optional_ptr<hal::serial> console;
  try {
    auto v5_console = resources::v5_console(512);
    console =
      hal::make_serial_converter(resources::driver_allocator(), v5_console);
  } catch (...) {
    console = resources::console();
  }
  hal::print(*console, "CANUSB Application Starting...\n\n"sv);

  auto serial = resources::usb_serial();
  auto canusb =
    hal::expander::canusb::create(resources::driver_allocator(), serial);

  auto manager =
    hal::acquire_can_bus_manager(resources::driver_allocator(), canusb);
  auto transceiver =
    hal::acquire_can_transceiver(resources::driver_allocator(), canusb, 32);

  manager->baud_rate(1_MHz);
  manager->filter_mode(hal::v5::can_bus_manager::accept::all);
  manager->bus_on();

  auto const receive_buffer = transceiver->receive_buffer();
  auto previous_cursor = transceiver->receive_cursor();

  while (true) {
    auto const cursor = transceiver->receive_cursor();

    resources::sleep(1s);

    transceiver->send({
      .id = 0x111,
      .length = 3,
      .payload = { 0xAB, 0xCD, 0xEF },
    });

    if (cursor == previous_cursor) {
      continue;
    }

    hal::print(*console, "Received: \n"sv);

    if (cursor < previous_cursor) {
      for (auto const& message : receive_buffer.subspan(previous_cursor)) {
        hal::print<32>(*console, "   id: 0x%08" PRIX32 "\n", message.id);
        hal::print<32>(*console, "  len: 0x%08" PRIX32 "\n", message.length);
      }
      previous_cursor = 0;
    }

    auto const delta = cursor - previous_cursor;
    for (auto const& message : receive_buffer.subspan(previous_cursor, delta)) {
      hal::print<32>(*console, "   id: 0x%08" PRIX32 "\n", message.id);
      hal::print<32>(*console, "  len: 0x%08" PRIX32 "\n", message.length);
      hal::print(*console, " data: "sv);
      for (auto const& byte :
           std::span(message.payload).first(message.length)) {
        hal::print<32>(*console, "0x%02" PRIX8 " ", byte);
      }
      hal::print(*console, "\n"sv);
    }

    previous_cursor = cursor;
  }
}
