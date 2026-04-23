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
tca9548a::tca9548a(hal::i2c& p_i2c, std::uint8_t p_address)
  : m_i2c(&p_i2c)
{
  // calculate address byte, only pay attention to last 3 bits
  auto mask = 0b111;
  auto masked_bits = p_address & mask;
  m_address = 0b01110000 | masked_bits;
}

hal::byte tca9548a::get_control_register_byte()
{
  return hal::read<1>(*m_i2c, m_address)[0];
}

void tca9548a::set_ports(std::bitset<8> p_ports)
{
  hal::write(
    *m_i2c,
    m_address,
    std::array<hal::byte, 1>{ static_cast<uint8_t>(p_ports.to_ulong()) });
}

std::bitset<8> tca9548a::get_ports_status()
{
  auto const control_register = get_control_register_byte();
  return control_register;
}

}  // namespace hal::expander
