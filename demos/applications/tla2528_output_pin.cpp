#include <array>

#include <libhal-expander/tla2528.hpp>
#include <libhal-expander/tla2528_adapters.hpp>
#include <libhal-util/bit.hpp>
#include <libhal-util/i2c.hpp>
#include <libhal-util/serial.hpp>
#include <libhal-util/steady_clock.hpp>
#include <libhal/output_pin.hpp>
#include <libhal/units.hpp>

#include <resource_list.hpp>

using namespace hal::literals;
using namespace std::chrono_literals;

void application(resource_list& p_map)
{
  constexpr bool demo_open_drain = false;

  auto& terminal = *p_map.console.value();
  auto& i2c = *p_map.i2c.value();
  auto& steady_clock = *p_map.clock.value();
  auto gpo_expander = hal::expander::tla2528(i2c);
  constexpr hal::output_pin::settings output_pin_config = {
    .resistor = hal::pin_resistor::none, .open_drain = demo_open_drain
  };
  std::array<hal::expander::tla2528_output_pin, 8> gpos{
    make_output_pin(gpo_expander, 0, output_pin_config),
    make_output_pin(gpo_expander, 1, output_pin_config),
    make_output_pin(gpo_expander, 2, output_pin_config),
    make_output_pin(gpo_expander, 3, output_pin_config),
    make_output_pin(gpo_expander, 4, output_pin_config),
    make_output_pin(gpo_expander, 5, output_pin_config),
    make_output_pin(gpo_expander, 6, output_pin_config),
    make_output_pin(gpo_expander, 7, output_pin_config)
  };

  // output counts in binary to go though all out put combinations
  hal::byte counter = 0;
  hal::print(terminal, "Starting Binary Count\n");
  while (true) {
    counter++;
    for (int i = 0; i < 8; i++) {
      gpos[i].level(hal::bit_extract(hal::bit_mask::from(i), counter));
    }
    hal::print<16>(terminal, "count:%x\n", counter);
    hal::delay(steady_clock, 200ms);
  }
}
