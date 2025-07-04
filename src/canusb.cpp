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

#include <libhal-expander/canusb.hpp>

#include <cinttypes>

#include <array>
#include <charconv>
#include <cstddef>
#include <libhal/pointers.hpp>
#include <optional>
#include <span>
#include <string_view>

#include <libhal-util/as_bytes.hpp>
#include <libhal/error.hpp>

namespace hal::expander {

namespace {

/**
 * @brief Convert baud rate to CANUSB setup command character
 */
char baud_rate_to_command_char(hal::u32 p_baud_rate)
{
  switch (p_baud_rate) {
    case 10000:
      return '0';
    case 20000:
      return '1';
    case 50000:
      return '2';
    case 100000:
      return '3';
    case 125000:
      return '4';
    case 250000:
      return '5';
    case 500000:
      return '6';
    case 800000:
      return '7';
    case 1000000:
      return '8';
    default:
      return '\0';
  }
}

/**
 * @brief Convert span of bytes to char span for parsing
 */
std::span<char const> to_chars(std::span<hal::byte const> p_data)
{
  return { reinterpret_cast<char const*>(p_data.data()), p_data.size() };
}

/**
 * @brief Parse CANUSB protocol string to CAN message
 */
std::optional<hal::can_message> string_to_can_message(
  std::span<hal::byte const> p_command)
{
  if (p_command.empty()) {
    return std::nullopt;
  }

  hal::can_message message{};
  std::size_t format_size = 0;
  std::size_t id_byte_length = 0;
  auto const command = p_command[0];
  auto command_chars = to_chars(p_command);

  // Determine message type and expected format
  if (command == 'r' || command == 't') {
    constexpr std::string_view format = "tiiil\r";
    format_size = format.size();
    id_byte_length = 3;
    message.extended = false;
  } else if (command == 'R' || command == 'T') {
    constexpr std::string_view format = "Tiiiiiiiil\r";
    format_size = format.size();
    id_byte_length = 8;
    message.extended = true;
  } else {
    return std::nullopt;
  }

  if (command_chars.size() < format_size) {
    return std::nullopt;
  }

  // Set remote request flag
  message.remote_request = (command == 'r' || command == 'R');

  // Skip first character (command)
  command_chars = command_chars.subspan(1);

  // Parse ID
  {
    hal::u32 id = 0;
    auto const status = std::from_chars(
      &command_chars[0], &command_chars[id_byte_length], id, 16);

    if (status.ec != std::errc{}) {
      return std::nullopt;
    }
    message.id = id;
  }

  // Move past ID field
  command_chars = command_chars.subspan(id_byte_length);

  // Parse length
  std::size_t payload_length = command_chars[0] - '0';
  if (payload_length > 8) {
    return std::nullopt;
  }

  // Move past length character
  command_chars = command_chars.subspan(1);

  // Verify expected remaining length
  std::size_t expected_remaining = (payload_length * 2) + 1;  // +1 for '\r'
  if (command_chars.size() != expected_remaining) {
    return std::nullopt;
  }

  message.length = payload_length;

  // Parse payload data
  for (std::size_t i = 0; i < payload_length; i++) {
    std::size_t character_offset = i * 2;
    auto status = std::from_chars(&command_chars[character_offset],
                                  &command_chars[character_offset + 2],
                                  message.payload[i],
                                  16);
    if (status.ec != std::errc{}) {
      return std::nullopt;
    }
  }

  return message;
}

/**
 * @brief Fixed-size buffer for CANUSB protocol strings
 *
 * Maximum size calculation:
 * - Command: 1 byte ('T')
 * - Extended ID: 8 bytes ("12345678")
 * - Length: 1 byte ('8')
 * - Data: 16 bytes ("0123456789ABCDEF")
 * - Terminator: 1 byte ('\r')
 * Total: 27 bytes maximum, so 28 bytes is sufficient
 */
struct canusb_command_buffer
{
  std::array<hal::byte, 28> data{};
  std::size_t size = 0;

  void push_back(hal::byte p_byte)
  {
    if (size < data.size()) {
      data[size++] = p_byte;
    }
  }

  void append(std::span<char> p_str)
  {
    for (char c : p_str) {
      push_back(static_cast<hal::byte>(c));
    }
  }

  [[nodiscard]] std::span<hal::byte const> span() const
  {
    return { data.data(), size };
  }
};

/**
 * @brief Convert CAN message to CANUSB protocol command buffer
 */
canusb_command_buffer can_message_to_command_buffer(
  hal::can_message const& p_message)
{
  canusb_command_buffer result;

  if (p_message.extended) {
    if (p_message.remote_request) {
      result.push_back('R');
    } else {
      result.push_back('T');
    }
    // Extended ID - 8 hex digits
    std::array<char, 9> id_buffer;
    std::snprintf(
      id_buffer.data(), id_buffer.size(), "%08" PRIX32, p_message.id);
    result.append(id_buffer);
  } else {
    if (p_message.remote_request) {
      result.push_back('r');
    } else {
      result.push_back('t');
    }
    // Standard ID - 3 hex digits
    std::array<char, 4> id_buffer;
    std::snprintf(
      id_buffer.data(), id_buffer.size(), "%03" PRIX32, p_message.id);
    result.append(std::span(id_buffer).first(3));
  }

  // Add length
  result.push_back('0' + p_message.length);

  // Add data bytes (if not remote request)
  if (!p_message.remote_request) {
    for (std::size_t i = 0; i < p_message.length; i++) {
      std::array<char, 3> byte_buffer;
      std::snprintf(byte_buffer.data(),
                    byte_buffer.size(),
                    "%02" PRIX8,
                    p_message.payload[i]);
      result.append(std::span(byte_buffer).first(2));
    }
  }

  result.push_back('\r');
  return result;
}

}  // anonymous namespace

class canusb_bus_manager : public hal::v5::can_bus_manager
{
public:
  canusb_bus_manager(hal::v5::strong_ptr<canusb> const& p_manager)
    : m_manager(p_manager)
  {
  }

private:
  friend class canusb;
  void driver_baud_rate(hal::u32 p_hertz) override;
  void driver_filter_mode(accept p_accept) override;
  void driver_on_bus_off(optional_bus_off_handler& p_callback) override;
  void driver_bus_on() override;
  hal::v5::strong_ptr<canusb> m_manager;
  optional_bus_off_handler m_bus_off_handler;
};

class canusb_transceiver : public hal::can_transceiver
{
public:
  canusb_transceiver(hal::v5::strong_ptr<canusb> const& p_manager,
                     std::pmr::polymorphic_allocator<> p_allocator,
                     hal::usize p_capacity)
    : m_manager(p_manager)
    , m_circular_buffer(p_allocator, p_capacity)
  {
  }

private:
  friend class canusb;
  void process_incoming_serial_data();

  u32 driver_baud_rate() override;
  void driver_send(hal::can_message const& p_message) override;
  std::span<hal::can_message const> driver_receive_buffer() override;
  hal::usize driver_receive_cursor() override;

  hal::v5::strong_ptr<canusb> m_manager;
  hal::v5::circular_buffer<hal::can_message> m_circular_buffer;
  hal::usize m_last_serial_cursor = 0;
  std::array<hal::byte, 32> m_parse_buffer{};
  hal::usize m_parse_buffer_pos = 0;
};

// ============================================================================
// canusb implementation
// ============================================================================

hal::v5::strong_ptr<canusb> canusb::create(
  std::pmr::polymorphic_allocator<> p_allocator,
  hal::v5::strong_ptr<hal::v5::serial> const& p_serial)
{
  return hal::v5::make_strong_ptr<canusb>(p_allocator, p_serial);
}

canusb::canusb(hal::v5::strong_ptr_only_token,
               hal::v5::strong_ptr<hal::v5::serial> const& p_serial)
  : m_serial(p_serial)
{
}

hal::v5::strong_ptr<v5::can_bus_manager> acquire_can_bus_manager(
  std::pmr::polymorphic_allocator<> p_allocator,
  hal::v5::strong_ptr<canusb> const& p_manager)
{
  if (p_manager->m_bus_manager_acquired) {
    hal::safe_throw(hal::device_or_resource_busy(&(*p_manager)));
  }

  p_manager->m_bus_manager_acquired = true;

  return hal::v5::make_strong_ptr<canusb_bus_manager>(p_allocator, p_manager);
}

hal::v5::strong_ptr<hal::can_transceiver> acquire_can_transceiver(
  std::pmr::polymorphic_allocator<> p_allocator,
  hal::v5::strong_ptr<canusb> const& p_manager,
  hal::usize p_buffer_size)
{
  if (p_manager->m_transceiver_acquired) {
    hal::safe_throw(hal::device_or_resource_busy(&(*p_manager)));
  }

  p_manager->m_transceiver_acquired = true;
  auto driver = hal::v5::make_strong_ptr<canusb_transceiver>(
    p_allocator, p_manager, p_allocator, p_buffer_size);
  return driver;
}

// ============================================================================
// canusb_bus_manager implementation
// ============================================================================

void canusb_bus_manager::driver_baud_rate(hal::u32 p_hertz)
{
  if (m_manager->m_is_open) {
    hal::safe_throw(hal::operation_not_permitted(this));
  }

  char command_char = baud_rate_to_command_char(p_hertz);

  if (command_char == '\0') {
    hal::safe_throw(hal::operation_not_supported(this));
  }

  // Send setup command: "SX\r" where X is the baud rate character
  std::array<hal::byte, 3> command = { 'S',
                                       static_cast<hal::byte>(command_char),
                                       '\r' };
  m_manager->m_serial->write(command);

  m_manager->m_current_baud_rate = p_hertz;
}

void canusb_bus_manager::driver_filter_mode(accept)
{
  // Filter mode does nothing as specified in the requirements
}

void canusb_bus_manager::driver_on_bus_off(optional_bus_off_handler& p_callback)
{
  // Store the callback but CANUSB protocol doesn't provide bus-off
  // notifications
  m_bus_off_handler = p_callback;
}

void canusb_bus_manager::driver_bus_on()
{
  if (m_manager->m_is_open) {
    return;  // Already open
  }

  // Send open command: "O\r"
  std::array<hal::byte, 2> command = { 'O', '\r' };
  m_manager->m_serial->write(command);

  m_manager->m_is_open = true;
}

// ============================================================================
// canusb_transceiver implementation
// ============================================================================

hal::u32 canusb_transceiver::driver_baud_rate()
{
  return m_manager->m_current_baud_rate;
}

void canusb_transceiver::driver_send(hal::can_message const& p_message)
{
  if (not m_manager->m_is_open) {
    hal::safe_throw(hal::operation_not_supported(this));
  }

  auto const command_str = can_message_to_command_buffer(p_message);
  m_manager->m_serial->write(command_str.span());
}

std::span<hal::can_message const> canusb_transceiver::driver_receive_buffer()
{
  // Process any new serial data when this method is called
  process_incoming_serial_data();

  return { m_circular_buffer.data(), m_circular_buffer.capacity() };
}

std::size_t canusb_transceiver::driver_receive_cursor()
{
  // Process any new serial data when this method is called
  process_incoming_serial_data();

  return m_circular_buffer.write_index();
}

void canusb_transceiver::process_incoming_serial_data()
{
  auto serial_buffer = m_manager->m_serial->receive_buffer();
  auto current_cursor = m_manager->m_serial->receive_cursor();

  // Calculate how much new data has arrived
  auto buffer_size = serial_buffer.size();
  auto bytes_received =
    (current_cursor + buffer_size - m_last_serial_cursor) % buffer_size;

  if (bytes_received == 0) {
    return;  // No new data
  }

  // Process new bytes
  for (std::size_t i = 0; i < bytes_received; i++) {
    auto byte_index = (m_last_serial_cursor + i) % buffer_size;
    hal::byte new_byte = serial_buffer[byte_index];

    // Add byte to parse buffer
    if (m_parse_buffer_pos < m_parse_buffer.size() - 1) {
      m_parse_buffer[m_parse_buffer_pos++] = new_byte;
    }

    // Check for end of message
    if (new_byte == '\r') {
      // Try to parse the complete message
      auto message_span =
        std::span<hal::byte const>(m_parse_buffer.data(), m_parse_buffer_pos);
      auto parsed_message = string_to_can_message(message_span);

      if (parsed_message) {
        // Successfully parsed, add to circular buffer
        m_circular_buffer.push(*parsed_message);
      }

      // Reset parse buffer for next message
      m_parse_buffer_pos = 0;
    }
  }
  // Update our cursor position
  m_last_serial_cursor = current_cursor;
}
}  // namespace hal::expander
