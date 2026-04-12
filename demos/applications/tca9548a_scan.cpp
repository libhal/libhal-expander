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

#include <libhal-expander/tca9548a.hpp>
#include <libhal-util/i2c.hpp>
#include <libhal-util/serial.hpp>
#include <libhal-util/steady_clock.hpp>

#include <resource_list.hpp>

void application()
{
  using namespace std::chrono_literals;
  using namespace hal::literals;

  auto console = resources::console();
  auto clock = resources::clock();
  auto i2c = resources::i2c();
  hal::expander::tca9548a i2c_mux = tca9548a(*i2c);

  hal::print(*console, "I2c scanner starting!");

  while (true) {
    using namespace std::literals;
    for (std::uint8_t i = 0; i < 8; i++) {
      i2c_mux.enable_port(i);
      constexpr hal::byte first_i2c_address = 0x08;
      constexpr hal::byte last_i2c_address = 0x78;

      hal::print<32>(*console, "\nDevices Found on port %d: ", i);

      for (hal::byte address = first_i2c_address; address < last_i2c_address;
           address++) {
        // This can only fail if the device is not present
        if (hal::probe(*i2c, address)) {
          hal::print<12>(*console, "0x%02X ", address);
        }
      }
    }
    hal::delay(*clock, 1s);
  }
}
