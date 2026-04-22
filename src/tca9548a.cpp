// Copyright 2026 Malia Labor and the libhal contributors
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

namespace hal::expander {
tca9548a(hal::i2c& p_i2c, u8 p_address = 0b01110000);
  : m_i2c(&p_i2c)
  {
    // calculate address byte, only pay attention to last 3 bits
    auto mask = 0b111;
    auto masked_bits = p_address & mask;
    m_address = 0b01110000 | masked_bits;
  }

  hal::byte tca9548a::get_control_register_byte()
  {
    std::array<hal::byte, 1> response{};
    return hal::read<1>(*m_i2c, m_address, response)[0];
  }

  void tca9548a::enable_port(std::uint8_t p_port_number)
  {
    if (p_port_number < 8) {
      hal::byte byte_to_send = 0x00;
      byte_to_send = byte_to_send | (1 << p_port_number);
      hal::write(*m_i2c, m_address, std::array<hal::byte, 1>{ byte_to_send });
    }
  }

  void tca9548a::enable_multiple_ports(std::bitset<8> p_ports)
  {
    hal::write(*m_i2c, m_address, std::array<hal::byte, 1>{ p_ports });
  }

  void tca9548a::disable_port(std::uint8_t p_port_number)
  {
    hal::byte byte_to_send = get_control_register_byte();
    if (p_port_number < 8) {
      hal::byte bit_select = 1 << p_port_number;
      hal::byte bit_mask = ~bit_select;
      byte_to_send = byte_to_send & bit_mask;
    }
    hal::write(*m_i2c, m_address, std::array<hal::byte, 1>{ byte_to_send });
  }

  std::bitset<8> tca9548a::get_ports_status()
  {
    auto const control_register = get_control_register_byte();
    return control_register;
  }

  }  // namespace hal::expander
