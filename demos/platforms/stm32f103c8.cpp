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

#include <memory_resource>

#include <libhal-arm-mcu/dwt_counter.hpp>
#include <libhal-arm-mcu/startup.hpp>
#include <libhal-arm-mcu/stm32f1/adc.hpp>
#include <libhal-arm-mcu/stm32f1/can2.hpp>
#include <libhal-arm-mcu/stm32f1/clock.hpp>
#include <libhal-arm-mcu/stm32f1/constants.hpp>
#include <libhal-arm-mcu/stm32f1/gpio.hpp>
#include <libhal-arm-mcu/stm32f1/independent_watchdog.hpp>
#include <libhal-arm-mcu/stm32f1/input_pin.hpp>
#include <libhal-arm-mcu/stm32f1/output_pin.hpp>
#include <libhal-arm-mcu/stm32f1/pin.hpp>
#include <libhal-arm-mcu/stm32f1/spi.hpp>
#include <libhal-arm-mcu/stm32f1/timer.hpp>
#include <libhal-arm-mcu/stm32f1/uart.hpp>
#include <libhal-arm-mcu/stm32f1/usart.hpp>
#include <libhal-arm-mcu/stm32f1/usb.hpp>
#include <libhal-arm-mcu/system_control.hpp>
#include <libhal-util/atomic_spin_lock.hpp>
#include <libhal-util/bit_bang_i2c.hpp>
#include <libhal-util/bit_bang_spi.hpp>
#include <libhal-util/inert_drivers/inert_adc.hpp>
#include <libhal-util/serial.hpp>
#include <libhal-util/steady_clock.hpp>
#include <libhal/pointers.hpp>
#include <libhal/pwm.hpp>
#include <libhal/units.hpp>

#include <libhal-arm-mcu/dwt_counter.hpp>
#include <libhal-arm-mcu/startup.hpp>
#include <libhal-arm-mcu/stm32f1/clock.hpp>
#include <libhal-arm-mcu/stm32f1/constants.hpp>
#include <libhal-arm-mcu/stm32f1/output_pin.hpp>
#include <libhal-arm-mcu/stm32f1/uart.hpp>
#include <libhal-arm-mcu/system_control.hpp>
#include <libhal-exceptions/control.hpp>
#include <libhal-util/steady_clock.hpp>

#include <resource_list.hpp>

using st_peripheral = hal::stm32f1::peripheral;

hal::v5::optional_ptr<hal::steady_clock> clock_ptr;
hal::v5::optional_ptr<hal::serial> console_ptr;
hal::v5::optional_ptr<hal::output_pin> status_led_ptr;
hal::v5::optional_ptr<hal::i2c> i2c_ptr;
hal::v5::optional_ptr<hal::v5::serial> usb_serial_ptr;
hal::v5::optional_ptr<hal::v5::serial> v5_console_ptr;
hal::v5::optional_ptr<hal::stm32f1::gpio<st_peripheral::gpio_a>> gpio_a_ptr;
hal::v5::optional_ptr<hal::stm32f1::gpio<st_peripheral::gpio_b>> gpio_b_ptr;
hal::v5::optional_ptr<hal::stm32f1::gpio<st_peripheral::gpio_c>> gpio_c_ptr;

[[noreturn]] void terminate_handler() noexcept
{
  if (not status_led_ptr && not clock_ptr) {
    // spin here until debugger is connected
    while (true) {
      continue;
    }
  }

  // Otherwise, blink the led in a pattern

  while (true) {
    using namespace std::chrono_literals;
    status_led_ptr->level(false);
    hal::delay(*clock_ptr, 100ms);
    status_led_ptr->level(true);
    hal::delay(*clock_ptr, 100ms);
    status_led_ptr->level(false);
    hal::delay(*clock_ptr, 100ms);
    status_led_ptr->level(true);
    hal::delay(*clock_ptr, 1000ms);
  }
}

void initialize_platform()
{
  using namespace hal::literals;
  hal::set_terminate(terminate_handler);
  // Set the MCU to the maximum clock speed using just the internal oscillator
  hal::stm32f1::maximum_speed_using_internal_oscillator();
}

namespace resources {
using namespace hal::literals;
std::array<hal::byte, 1024> driver_memory{};
std::pmr::monotonic_buffer_resource resource(driver_memory.data(),
                                             driver_memory.size(),
                                             std::pmr::null_memory_resource());
std::pmr::polymorphic_allocator<> driver_allocator()
{
  return &resource;
}

void reset()
{
  hal::cortex_m::reset();
}

void sleep(hal::time_duration p_duration)
{
  auto delay_clock = resources::clock();
  hal::delay(*delay_clock, p_duration);
}

hal::v5::strong_ptr<hal::steady_clock> clock()
{
  if (clock_ptr) {
    return clock_ptr;
  }

  clock_ptr = hal::v5::make_strong_ptr<hal::cortex_m::dwt_counter>(
    driver_allocator(), hal::stm32f1::frequency(hal::stm32f1::peripheral::cpu));

  return clock_ptr;
}

hal::v5::strong_ptr<hal::serial> console()
{
  if (console_ptr) {
    return console_ptr;
  }

  console_ptr =
    hal::v5::make_strong_ptr<hal::stm32f1::uart>(driver_allocator(),
                                                 hal::port<1>,
                                                 hal::buffer<128>,
                                                 hal::serial::settings{
                                                   .baud_rate = 115200,
                                                 });
  return console_ptr;
}

hal::v5::strong_ptr<hal::output_pin> status_led()
{
  if (status_led_ptr) {
    return status_led_ptr;
  }

  status_led_ptr = hal::v5::make_strong_ptr<hal::stm32f1::output_pin>(
    driver_allocator(), 'C', 13);

  return status_led_ptr;
}

hal::v5::strong_ptr<hal::stm32f1::gpio<st_peripheral::gpio_a>> gpio_a()
{
  if (not gpio_a_ptr) {
    gpio_a_ptr =
      hal::v5::make_strong_ptr<hal::stm32f1::gpio<st_peripheral::gpio_a>>(
        driver_allocator());
  }
  return gpio_a_ptr;
}

hal::v5::strong_ptr<hal::stm32f1::gpio<st_peripheral::gpio_b>> gpio_b()
{
  if (not gpio_b_ptr) {
    gpio_b_ptr =
      hal::v5::make_strong_ptr<hal::stm32f1::gpio<st_peripheral::gpio_b>>(
        driver_allocator());
  }
  return gpio_b_ptr;
}

hal::v5::strong_ptr<hal::stm32f1::gpio<st_peripheral::gpio_c>> gpio_c()
{
  if (not gpio_c_ptr) {
    gpio_c_ptr =
      hal::v5::make_strong_ptr<hal::stm32f1::gpio<st_peripheral::gpio_c>>(
        driver_allocator());
  }
  return gpio_c_ptr;
}

hal::v5::strong_ptr<hal::i2c> i2c()
{
  // TODO(#167): Use a version of bit_bang_i2c that accepts strong_ptr's
  static auto sda_output_pin =
    hal::acquire_output_pin(driver_allocator(), gpio_b(), 7);
  static auto scl_output_pin =
    hal::acquire_output_pin(driver_allocator(), gpio_b(), 6);
  auto clock = resources::clock();
  return hal::v5::make_strong_ptr<hal::bit_bang_i2c>(
    driver_allocator(),
    hal::bit_bang_i2c::pins{
      .sda = &(*sda_output_pin),
      .scl = &(*scl_output_pin),
    },
    *clock);
}

hal::v5::strong_ptr<hal::v5::serial> usb_serial()
{
  hal::safe_throw(hal::bad_optional_ptr_access(nullptr));
}

hal::v5::strong_ptr<hal::v5::serial> v5_console(hal::usize)
{
  hal::safe_throw(hal::bad_optional_ptr_access(nullptr));
}
}  // namespace resources
