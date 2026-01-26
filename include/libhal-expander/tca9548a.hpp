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

#pragma once

#include <cstdint>

#include <libhal/i2c.hpp>

namespace hal::expander {

class tca9548a
{
public:
  void enable_port(std::uint8_t p_port_number);
  void disable_port(std::uint8_t p_port_number);
  void set_multiple_ports(std::array<bool, 8> p_ports);

  std::array<bool, 8> get_ports_status();

  tca9548a(hal::i2c& p_i2c,
           bool p_addr_bit_0 = false,
           bool p_addr_bit_1 = false,
           bool p_addr_bit_2 = false);

private:
  hal::i2c* m_i2c;
  hal::byte m_address;
};
}  // namespace hal::expander
