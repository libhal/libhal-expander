#include <libhal-expander/tla2528_adapters.hpp>
#include <libhal-util/bit.hpp>
#include <libhal/error.hpp>
#include <libhal/input_pin.hpp>
#include <libhal/units.hpp>

namespace hal::expander {

tla2528_output_pin make_output_pin(tla2528& p_tla2528,
                                   hal::byte const p_channel,
                                   hal::output_pin::settings const& p_settings)
{
  return { p_tla2528, p_channel, p_settings };
}
tla2528_output_pin::tla2528_output_pin(
  tla2528& p_tla2528,
  hal::byte p_channel,
  hal::output_pin::settings const& p_settings)
  : m_tla2528(&p_tla2528)
  , m_channel(p_channel)
{
  m_tla2528->throw_if_channel_occupied(m_channel);
  tla2528_output_pin::driver_configure(p_settings);
  hal::bit_modify(m_tla2528->m_object_created)
    .set(hal::bit_mask::from(m_channel));
}
tla2528_output_pin::~tla2528_output_pin()
{
  hal::bit_modify(m_tla2528->m_object_created)
    .clear(hal::bit_mask::from(m_channel));
}
void tla2528_output_pin::driver_configure(
  hal::output_pin::settings const& p_settings)
{
  if (p_settings.resistor != hal::pin_resistor::none) {
    throw hal::operation_not_supported(this);
  }
  if (p_settings.open_drain) {
    m_tla2528->set_pin_mode(tla2528::pin_mode::output_pin_open_drain,
                            m_channel);
  } else {
    m_tla2528->set_pin_mode(tla2528::pin_mode::output_pin_push_pull, m_channel);
  }
}
void tla2528_output_pin::driver_level(bool p_high)
{
  m_tla2528->set_output_pin(m_channel, p_high);
}
bool tla2528_output_pin::driver_level()
{
  return m_tla2528->get_input_pin(m_channel);
}

tla2528_input_pin make_input_pin(tla2528& p_tla2528,
                                 hal::byte p_channel,
                                 hal::input_pin::settings const& p_settings)
{
  return { p_tla2528, p_channel, p_settings };
}
tla2528_input_pin::tla2528_input_pin(tla2528& p_tla2528,
                                     hal::byte p_channel,
                                     hal::input_pin::settings const& p_settings)
  : m_tla2528(&p_tla2528)
  , m_channel(p_channel)
{
  m_tla2528->throw_if_channel_occupied(m_channel);
  m_tla2528->set_pin_mode(tla2528::pin_mode::input_pin, m_channel);
  tla2528_input_pin::driver_configure(p_settings);
  hal::bit_modify(m_tla2528->m_object_created)
    .set(hal::bit_mask::from(m_channel));
}
tla2528_input_pin::~tla2528_input_pin()
{
  hal::bit_modify(m_tla2528->m_object_created)
    .clear(hal::bit_mask::from(m_channel));
}
bool tla2528_input_pin::driver_level()
{
  return m_tla2528->get_input_pin(m_channel);
}
void tla2528_input_pin::driver_configure(
  hal::input_pin::settings const& p_settings)
{
  if (p_settings.resistor != hal::pin_resistor::none) {
    throw hal::operation_not_supported(this);
  }
}

tla2528_adc make_adc(tla2528& p_tla2528, hal::byte p_channel)
{
  return { p_tla2528, p_channel };
}
tla2528_adc::~tla2528_adc()
{
  hal::bit_modify(m_tla2528->m_object_created)
    .clear(hal::bit_mask::from(m_channel));
}
tla2528_adc::tla2528_adc(tla2528& p_tla2528, hal::byte p_channel)
  : m_tla2528(&p_tla2528)
  , m_channel(p_channel)
{
  m_tla2528->throw_if_channel_occupied(m_channel);
  m_tla2528->set_pin_mode(tla2528::pin_mode::adc, m_channel);
  hal::bit_modify(m_tla2528->m_object_created)
    .set(hal::bit_mask::from(m_channel));
}
float tla2528_adc::driver_read()
{
  return m_tla2528->get_adc_reading(m_channel);
}
}  // namespace hal::expander
