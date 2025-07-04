#include <array>

#include <libhal-expander/tla2528.hpp>
#include <libhal-expander/tla2528_adapters.hpp>
#include <libhal-util/bit.hpp>
#include <libhal-util/i2c.hpp>
#include <libhal-util/serial.hpp>
#include <libhal-util/steady_clock.hpp>
#include <libhal/input_pin.hpp>
#include <libhal/units.hpp>

#include <resource_list.hpp>

using namespace hal::literals;
using namespace std::chrono_literals;

void application()
{
  auto console = resources::console();
  auto i2c = resources::i2c();
  auto steady_clock = resources::clock();
  auto gpi_expander = hal::expander::tla2528(*i2c);
  std::array<hal::expander::tla2528_input_pin, 8> gpis{
    make_input_pin(gpi_expander, 0), make_input_pin(gpi_expander, 1),
    make_input_pin(gpi_expander, 2), make_input_pin(gpi_expander, 3),
    make_input_pin(gpi_expander, 4), make_input_pin(gpi_expander, 5),
    make_input_pin(gpi_expander, 6), make_input_pin(gpi_expander, 7)
  };

  while (true) {
    hal::print(*console, "\nvalues:");
    for (int i = 0; i < 8; i++) {
      hal::print<4>(*console, "%x", gpis[i].level());
    }
    resources::sleep(500ms);
  }
}
