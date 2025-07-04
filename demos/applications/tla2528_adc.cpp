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
  auto adc_mux = hal::expander::tla2528(*i2c);

  std::array<hal::expander::tla2528_adc, 8> adcs{
    make_adc(adc_mux, 0), make_adc(adc_mux, 1), make_adc(adc_mux, 2),
    make_adc(adc_mux, 3), make_adc(adc_mux, 4), make_adc(adc_mux, 5),
    make_adc(adc_mux, 6), make_adc(adc_mux, 7)
  };

  while (true) {
    hal::print(*console, "\nvalues:\n");
    for (int i = 0; i < 8; i++) {
      hal::print<64>(*console, "%d:%f\n", i, adcs[i].read());
    }
    resources::sleep(500ms);
  }
}
