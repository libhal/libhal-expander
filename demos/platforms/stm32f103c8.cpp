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

#include <libhal/units.hpp>

#include <libhal-arm-mcu/dwt_counter.hpp>
#include <libhal-arm-mcu/startup.hpp>
#include <libhal-arm-mcu/stm32f1/clock.hpp>
#include <libhal-arm-mcu/stm32f1/constants.hpp>
#include <libhal-arm-mcu/stm32f1/output_pin.hpp>
#include <libhal-arm-mcu/stm32f1/uart.hpp>
#include <libhal-arm-mcu/system_control.hpp>
#include <libhal-exceptions/control.hpp>

#include <resource_list.hpp>

[[noreturn]] void terminate_handler() noexcept
{
  if (not resources::status_led() && not resources::status_led()) {
    // spin here until debugger is connected
    while (true) {
      continue;
    }
  }

  // Otherwise, blink the led in a pattern

  auto led = resources::status_led();
  auto clock = resources::clock();

  while (true) {
    using namespace std::chrono_literals;
    led->level(false);
    hal::delay(*clock, 100ms);
    led->level(true);
    hal::delay(*clock, 100ms);
    led->level(false);
    hal::delay(*clock, 100ms);
    led->level(true);
    hal::delay(*clock, 1000ms);
  }
}

resource_list initialize_platform()
{
  using namespace hal::literals;

  // Set the MCU to the maximum clock speed
  hal::stm32f1::maximum_speed_using_internal_oscillator();

  static hal::cortex_m::dwt_counter counter(
    hal::stm32f1::frequency(hal::stm32f1::peripheral::cpu));

  static hal::stm32f1::uart uart1(hal::port<1>,
                                  hal::buffer<128>,
                                  hal::serial::settings{
                                    .baud_rate = 115200,
                                  });

  static hal::stm32f1::output_pin led('C', 13);

  return {
    .reset = +[]() { hal::cortex_m::reset(); },
    .console = &uart1,
    .clock = &counter,
    .status_led = &led,
    // no i2c yet!
  };
}
