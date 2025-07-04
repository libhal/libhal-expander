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

#include <cstdio>

#include <atomic>
#include <chrono>
#include <memory_resource>
#include <thread>

#include <libhal-mac/serial.hpp>
#include <libhal/error.hpp>
#include <libhal/pointers.hpp>
#include <libhal/serial.hpp>
#include <libhal/steady_clock.hpp>
#include <libhal/units.hpp>

#include <resource_list.hpp>

namespace resources {

// Console Serial Implementation
class console_serial : public hal::v5::serial
{
public:
  console_serial(std::pmr::polymorphic_allocator<hal::byte> p_allocator,
                 hal::usize p_buffer_size)
    : m_allocator(p_allocator)
    , m_receive_buffer(p_buffer_size, hal::byte{ 0 }, p_allocator)
    , m_receive_thread(&console_serial::receive_thread_function, this)
  {
  }

  ~console_serial() override
  {
    m_stop_thread.store(true, std::memory_order_release);
    if (m_receive_thread.joinable()) {
      m_receive_thread.join();
    }
  }

private:
  void driver_configure(hal::v5::serial::settings const&) override
  {
    // Console doesn't need configuration - settings are ignored
  }

  void driver_write(std::span<hal::byte const> p_data) override
  {
    // Use fwrite to stdout for binary safety
    std::fwrite(p_data.data(), 1, p_data.size(), stdout);
    std::fflush(stdout);
  }

  std::span<hal::byte const> driver_receive_buffer() override
  {
    return m_receive_buffer;
  }

  hal::usize driver_cursor() override
  {
    return m_receive_cursor.load(std::memory_order_acquire);
  }

  void receive_thread_function()
  {
    while (!m_stop_thread.load(std::memory_order_acquire)) {
      int ch = std::getchar();

      if (ch != EOF) {
        auto current_cursor = m_receive_cursor.load(std::memory_order_acquire);
        m_receive_buffer[current_cursor] = static_cast<hal::byte>(ch);

        auto new_cursor = (current_cursor + 1) % m_receive_buffer.size();
        m_receive_cursor.store(new_cursor, std::memory_order_release);
      } else {
        // No data available, sleep briefly to avoid busy waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }
    }
  }

  std::pmr::polymorphic_allocator<hal::byte> m_allocator;
  std::pmr::vector<hal::byte> m_receive_buffer;
  std::atomic<hal::usize> m_receive_cursor{ 0 };
  std::atomic<bool> m_stop_thread{ false };
  std::thread m_receive_thread;
};

// Steady Clock Implementation
class chrono_steady_clock : public hal::v5::steady_clock
{
public:
  chrono_steady_clock()
    : m_start_time(std::chrono::steady_clock::now())
  {
  }

private:
  hal::v5::hertz driver_frequency() override
  {
    // std::chrono::steady_clock frequency is represented by its period
    using period = std::chrono::steady_clock::period;

    // Convert period (seconds per tick) to frequency (ticks per second)
    // frequency = 1 / period = period::den / period::num
    constexpr auto frequency_hz = period::den / period::num;

    return frequency_hz;
  }

  hal::u64 driver_uptime() override
  {
    auto now = std::chrono::steady_clock::now();
    auto duration = now - m_start_time;
    return duration.count();
  }

  std::chrono::steady_clock::time_point m_start_time;
};

// Steady Clock Implementation
class legacy_chrono_steady_clock : public hal::steady_clock
{
public:
  legacy_chrono_steady_clock()
    : m_start_time(std::chrono::steady_clock::now())
  {
  }

private:
  hal::hertz driver_frequency() override
  {
    // std::chrono::steady_clock frequency is represented by its period
    using period = std::chrono::steady_clock::period;

    // Convert period (seconds per tick) to frequency (ticks per second)
    // frequency = 1 / period = period::den / period::num
    constexpr auto frequency_hz = period::den / period::num;

    return static_cast<hal::hertz>(frequency_hz);
  }

  hal::u64 driver_uptime() override
  {
    auto const now = std::chrono::steady_clock::now();
    auto const duration = now - m_start_time;
    return duration.count();
  }

  std::chrono::steady_clock::time_point m_start_time;
};

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

hal::v5::optional_ptr<hal::serial> console_ptr;
hal::v5::optional_ptr<legacy_chrono_steady_clock> clock_ptr;
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
    hal::v5::make_strong_ptr<legacy_chrono_steady_clock>(driver_allocator());
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
  std::this_thread::sleep_for(50ms);
  // De-activate RTS (boot) line
  usb_serial_ptr->set_rts(false);
  std::this_thread::sleep_for(50ms);
  // De-activate DTR (reset) line to reset device
  usb_serial_ptr->set_dtr(false);
  std::this_thread::sleep_for(50ms);

  return usb_serial_ptr;
}

hal::v5::strong_ptr<hal::v5::serial> v5_console(hal::usize p_buffer_size)
{
  if (serial_console_ptr) {
    return serial_console_ptr;
  }

  serial_console_ptr = hal::v5::make_strong_ptr<console_serial>(
    driver_allocator(), driver_allocator(), p_buffer_size);
  return serial_console_ptr;
}
}  // namespace resources

void initialize_platform()
{
  // do nothing
}
