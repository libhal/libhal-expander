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

#include <exception>

#include <libhal-mac/serial.hpp>
#include <libhal/error.hpp>
#include <libhal/pointers.hpp>
#include <libhal/serial.hpp>
#include <libhal/units.hpp>

#include <memory_resource>
#include <resource_list.hpp>

resource_list initialize_platform()
{
  using namespace hal::literals;

  // std::set_terminate(terminate_handler);

  return {
    .reset = +[]() { exit(-1); },
  };
}

hal::v5::strong_ptr<hal::v5::serial> usb_serial()
{
  constexpr auto usb_serial_path = "/dev/tty.usbserial-59760073631";
  return hal::mac::serial::create(std::pmr::new_delete_resource(),
                                  usb_serial_path,
                                  1024,
                                  { .baud_rate = 115200 });
}

#include <atomic>
#include <chrono>
#include <cstdio>
#include <ratio>
#include <thread>

#include <libhal/pointers.hpp>
#include <libhal/serial.hpp>
#include <libhal/steady_clock.hpp>

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
    return std::span<hal::byte const>(m_receive_buffer);
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

    return hal::v5::hertz{ frequency_hz };
  }

  hal::u64 driver_uptime() override
  {
    auto now = std::chrono::steady_clock::now();
    auto duration = now - m_start_time;
    return duration.count();
  }

  std::chrono::steady_clock::time_point m_start_time;
};

// Factory Functions
hal::v5::strong_ptr<hal::v5::serial> serial_console(hal::usize p_buffer_size)
{
  return hal::v5::make_strong_ptr<console_serial>(
    std::pmr::new_delete_resource(),
    std::pmr::new_delete_resource(),
    p_buffer_size);
}

hal::v5::strong_ptr<hal::v5::steady_clock> steady_clock()
{
  return hal::v5::make_strong_ptr<chrono_steady_clock>(
    std::pmr::new_delete_resource());
}
