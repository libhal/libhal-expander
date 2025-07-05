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
#include <thread>

#include <libhal-mac/console.hpp>
#include <libhal-mac/serial.hpp>
#include <libhal-mac/steady_clock.hpp>
#include <libhal/error.hpp>
#include <libhal/pointers.hpp>
#include <libhal/serial.hpp>
#include <libhal/steady_clock.hpp>
#include <libhal/units.hpp>

#include <resource_list.hpp>

namespace resources {

void reset()
{
  exit(-1);
}

void sleep(hal::time_duration p_duration)
{
  std::this_thread::sleep_for(p_duration);
}

std::pmr::polymorphic_allocator<> driver_allocator()
{
  return std::pmr::new_delete_resource();
}

hal::v5::optional_ptr<hal::mac::console_serial> console_ptr;
hal::v5::optional_ptr<hal::mac::legacy_steady_clock> clock_ptr;
hal::v5::optional_ptr<hal::output_pin> status_led_ptr;
hal::v5::optional_ptr<hal::i2c> i2c_ptr;
hal::v5::optional_ptr<hal::mac::serial> usb_serial_ptr;
hal::v5::optional_ptr<hal::v5::serial> serial_console_ptr;
hal::v5::optional_ptr<hal::v5::steady_clock> steady_clock_ptr;

hal::v5::strong_ptr<hal::serial> console()
{
  throw hal::bad_optional_ptr_access(nullptr);
}

hal::v5::strong_ptr<hal::steady_clock> clock()
{
  if (clock_ptr) {
    return clock_ptr;
  }

  clock_ptr =
    hal::v5::make_strong_ptr<hal::mac::legacy_steady_clock>(driver_allocator());
  return clock_ptr;
}

hal::v5::strong_ptr<hal::output_pin> status_led()
{
  throw hal::bad_optional_ptr_access(nullptr);
}

hal::v5::strong_ptr<hal::i2c> i2c()
{
  throw hal::bad_optional_ptr_access(nullptr);
}

hal::v5::strong_ptr<hal::v5::serial> usb_serial()
{
  if (usb_serial_ptr) {
    return usb_serial_ptr;
  }
  // NOTE: Change this to the USB serial port path...
  constexpr auto usb_serial_path = "/dev/tty.usbserial-59760073631";
  usb_serial_ptr = hal::mac::serial::create(
    driver_allocator(), usb_serial_path, 1024, { .baud_rate = 115200 });
  using namespace std::literals;

  // Assert DTR and RTS
  usb_serial_ptr->set_control_signals(true, true);
  std::this_thread::sleep_for(500ms);
  // De-activate RTS (boot) line
  usb_serial_ptr->set_rts(false);
  std::this_thread::sleep_for(500ms);
  // De-activate DTR (reset) line to reset device
  usb_serial_ptr->set_dtr(false);
  std::this_thread::sleep_for(500ms);

  return usb_serial_ptr;
}

hal::v5::strong_ptr<hal::v5::serial> v5_console(hal::usize p_buffer_size)
{
  if (serial_console_ptr) {
    return serial_console_ptr;
  }

  serial_console_ptr = hal::v5::make_strong_ptr<hal::mac::console_serial>(
    driver_allocator(), driver_allocator(), p_buffer_size);
  return serial_console_ptr;
}
}  // namespace resources

void initialize_platform()
{
  // do nothing
}
