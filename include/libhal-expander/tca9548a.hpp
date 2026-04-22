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

#pragma once

#include <bitset>
#include <cstdint>

#include <libhal/i2c.hpp>

namespace hal::expander {
/**
 * @brief tca9548a driver: 8 channel i2c multiplexer using eight bidirectional
 * translating switches that can be controlled through the i2c bus
 *
 */
class tca9548a
{
public:
  /**
   * @brief Construct a new tca9548a driver object
   *
   * @param p_i2c - an i2c bus driver to communicate with
   * @param p_address - address of the mux configured through address pins on
   * chip,
   */
  tca9548a(hal::i2c& p_i2c, u8 p_address = 0b01110000);

  /**
   * @brief Enable a single port to read and write to over i2c.
   *
   * If provided  number is greater than 7, no port configuration byte is sent.
   * This preseerves the state of the ports before attempting to set a port out
   * of range.
   *
   * @param p_port_number - number of port to enable, 0 - 7
   */
  void enable_port(std::uint8_t p_port_number);

  /**
   * @brief Enable a multiple ports to read and write to over i2c
   *
   * @param p_ports bitset representing state of each port
   */
  void enable_multiple_ports(std::bitset<8> p_ports);

  /**
   * @brief Disable a single port while leaving other ports in their current
   * state
   *
   * @param p_port_number - number of port to disable, 0 - 7
   */
  void disable_port(std::uint8_t p_port_number);

  /**
   * @brief Get the status of each port
   *
   * @return std::array<bool, 8> - array of bools representing which ports are
   * on or off
   */
  std::bitset<8> get_ports_status();

private:
  hal::byte get_control_register_byte();

  hal::i2c* m_i2c;
  hal::byte m_address = 0x70;
  // TODO(#35): Add i2c drivers that automatically handle port switching
  hal::byte m_port_ownership = 0;       // reserved for future usage
  hal::byte m_current_port_cached = 0;  // reserved for future usage
};
}  // namespace hal::expander
