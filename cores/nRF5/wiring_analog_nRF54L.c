/*
  Copyright (c) 2014 Arduino LLC.  All right reserved.
  Copyright (c) 2016 Sandeep Mistry All right reserved.
  Copyright (c) 2026 Thomas Goettgens - nRF54L port.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#if defined(NRF54L_SERIES)

#include "nrf.h"

#include "Arduino.h"
#include "wiring_private.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * nRF54L SAADC differences from nRF52 (same MMIO struct, different semantics):
 *
 *   - REFSEL has only two values: Internal (0.9 V) and External (1.2 V via
 *     the PADC_EXT_REF_1V2 pin). There is no VDD/4 reference. The Arduino
 *     analogReference() modes are mapped to the closest equivalent
 *     reference + gain combination; ranges below are approximate.
 *
 *   - GAIN enum is now multipliers (2, 1, 2/3, 2/4, 2/5, 2/6, 2/7, 2/8)
 *     instead of dividers (1/6, 1/5, ..., 1, 2, 4). The full-scale input
 *     range = Vref / gain, so for the internal 0.9 V reference:
 *         Gain2_8 (0.25) -> 0..3.6 V (default range)
 *         Gain2_7 (~0.286) -> 0..3.15 V
 *         Gain2_6 (~0.333) -> 0..2.7 V
 *         Gain2_5 (0.4)    -> 0..2.25 V
 *         Gain2_4 (0.5)    -> 0..1.8 V
 *         Gain2_3 (~0.667) -> 0..1.35 V
 *         Gain1   (1.0)    -> 0..0.9 V
 *         Gain2   (2.0)    -> 0..0.45 V
 *
 *   - TACQ is no longer a 3/5/10/15/20/40 us enum. CH[n].CONFIG.TACQ is a
 *     count of 125 ns steps: acquisition time = (TACQ + 1) x 125 ns, for
 *     TACQ = 1..0x13F (SAADC_CH_CONFIG_TACQ_Min/Max), i.e. 250 ns..40 us.
 *     analogSampleTime() keeps taking microseconds and converts.
 *
 *   - RESULT.MAXCNT is now in BYTES, not 16-bit samples. For a single
 *     sample we must write sizeof(int16_t) = 2 here, not 1.
 *
 *   - PSELP is a structured bitfield (CONNECT[31:30] | PORT[11:8] | PIN[4:0])
 *     instead of an "AnalogInput0..7" enum. Any GPIO that supports SAADC
 *     routing can be selected by its port + pin number.
 */

static uint32_t saadcReference = SAADC_CH_CONFIG_REFSEL_Internal;
static uint32_t saadcGain      = SAADC_CH_CONFIG_GAIN_Gain2_8;
// CH[n].CONFIG.TACQ for an acquisition time in whole microseconds:
// (TACQ + 1) x 125 ns = us  =>  TACQ = us * 8 - 1.
#define TACQ_FROM_US(us)     ((uint32_t)(us) * 8UL - 1UL)
// 3 us, the nRF52 core's default, for low-impedance sources (the nRF52 PS
// rates 3 us for up to 10 kohm). A high-impedance source such as a battery
// divider wants analogSampleTime(40).
#define SAMPLE_TIME_DEFAULT_US  3
static uint32_t saadcTacq = TACQ_FROM_US(SAMPLE_TIME_DEFAULT_US);
static bool     saadcBurst     = false;

static int readResolution = 10;

void analogReadResolution( int res )
{
  readResolution = res;
}

static inline uint32_t mapResolution( uint32_t value, uint32_t from, uint32_t to )
{
  if ( from == to ) return value;
  if ( from >  to ) return value >> (from - to);
  return value << (to - from);
}

void analogReference( eAnalogReference ulMode )
{
  // All modes use the internal 0.9 V reference; only the gain changes.
  // VDD/4 isn't available on nRF54L SAADC, so AR_VDD4 falls back to the
  // widest internal range.
  saadcReference = SAADC_CH_CONFIG_REFSEL_Internal;

  switch ( ulMode )
  {
    case AR_INTERNAL_3_0:  saadcGain = SAADC_CH_CONFIG_GAIN_Gain2_7; break; // ~3.15 V
    case AR_INTERNAL_2_4:  saadcGain = SAADC_CH_CONFIG_GAIN_Gain2_5; break; // 2.25 V
    case AR_INTERNAL_1_8:  saadcGain = SAADC_CH_CONFIG_GAIN_Gain2_4; break; // 1.8 V
    case AR_INTERNAL_1_2:  saadcGain = SAADC_CH_CONFIG_GAIN_Gain2_3; break; // ~1.35 V
    case AR_VDD4:          // no real VDD/4 ref; pick widest internal range
    case AR_DEFAULT:
    case AR_INTERNAL:
    default:               saadcGain = SAADC_CH_CONFIG_GAIN_Gain2_8; break; // 3.6 V
  }
}

void analogOversampling( uint32_t ulOversampling )
{
  saadcBurst = true;

  switch ( ulOversampling )
  {
    case 0:
    case 1:
      saadcBurst = false;
      NRF_SAADC->OVERSAMPLE = SAADC_OVERSAMPLE_OVERSAMPLE_Bypass;
      return;
    case 2:   NRF_SAADC->OVERSAMPLE = SAADC_OVERSAMPLE_OVERSAMPLE_Over2x;   break;
    case 4:   NRF_SAADC->OVERSAMPLE = SAADC_OVERSAMPLE_OVERSAMPLE_Over4x;   break;
    case 8:   NRF_SAADC->OVERSAMPLE = SAADC_OVERSAMPLE_OVERSAMPLE_Over8x;   break;
    case 16:  NRF_SAADC->OVERSAMPLE = SAADC_OVERSAMPLE_OVERSAMPLE_Over16x;  break;
    case 32:  NRF_SAADC->OVERSAMPLE = SAADC_OVERSAMPLE_OVERSAMPLE_Over32x;  break;
    case 64:  NRF_SAADC->OVERSAMPLE = SAADC_OVERSAMPLE_OVERSAMPLE_Over64x;  break;
    case 128: NRF_SAADC->OVERSAMPLE = SAADC_OVERSAMPLE_OVERSAMPLE_Over128x; break;
    case 256: NRF_SAADC->OVERSAMPLE = SAADC_OVERSAMPLE_OVERSAMPLE_Over256x; break;
  }
}

void analogSampleTime( uint8_t sTime )
{
  // sTime is in microseconds, as on nRF52, so the legacy 3/5/10/15/20/40
  // values keep their meaning; any whole value in between works too. 0 picks
  // the default, and anything past 40 us clamps to TACQ_Max (= 40 us).
  uint32_t tacq = TACQ_FROM_US(sTime ? sTime : SAMPLE_TIME_DEFAULT_US);
  if (tacq > SAADC_CH_CONFIG_TACQ_Max) tacq = SAADC_CH_CONFIG_TACQ_Max;
  saadcTacq = tacq;
}

static uint32_t analogRead_internal( uint32_t pselp )
{
  uint32_t saadcResolution;
  uint32_t resolution;
  // EasyDMA writes this asynchronously; keep it volatile so the compiler
  // doesn't elide the read after TASKS_SAMPLE.
  volatile int16_t value = 0;

  if      (readResolution <= 8)  { resolution = 8;  saadcResolution = SAADC_RESOLUTION_VAL_8bit; }
  else if (readResolution <= 10) { resolution = 10; saadcResolution = SAADC_RESOLUTION_VAL_10bit; }
  else if (readResolution <= 12) { resolution = 12; saadcResolution = SAADC_RESOLUTION_VAL_12bit; }
  else                           { resolution = 14; saadcResolution = SAADC_RESOLUTION_VAL_14bit; }

  NRF_SAADC->RESOLUTION = saadcResolution;

  NRF_SAADC->ENABLE = (SAADC_ENABLE_ENABLE_Enabled << SAADC_ENABLE_ENABLE_Pos);

  // Park all 8 channels at "not connected" then configure CH[0] for this read.
  for (int i = 0; i < 8; i++) {
    NRF_SAADC->CH[i].PSELN = (SAADC_CH_PSELP_CONNECT_NC << SAADC_CH_PSELP_CONNECT_Pos);
    NRF_SAADC->CH[i].PSELP = (SAADC_CH_PSELP_CONNECT_NC << SAADC_CH_PSELP_CONNECT_Pos);
  }

  // nRF54L SAADC CH.CONFIG drops the RESP/RESN pull-resistor fields that
  // nRF52 had; remaining fields are GAIN, REFSEL, MODE, TACQ, TCONV, plus
  // BURST on the chips that keep it per-channel.
  NRF_SAADC->CH[0].CONFIG =
        ((saadcGain                      << SAADC_CH_CONFIG_GAIN_Pos)   & SAADC_CH_CONFIG_GAIN_Msk)
      | ((saadcReference                 << SAADC_CH_CONFIG_REFSEL_Pos) & SAADC_CH_CONFIG_REFSEL_Msk)
      | ((saadcTacq                      << SAADC_CH_CONFIG_TACQ_Pos)   & SAADC_CH_CONFIG_TACQ_Msk)
      | ((SAADC_CH_CONFIG_MODE_SE        << SAADC_CH_CONFIG_MODE_Pos)   & SAADC_CH_CONFIG_MODE_Msk)
#if defined(SAADC_CH_CONFIG_BURST_Msk)
      | (((saadcBurst ? SAADC_CH_CONFIG_BURST_Enabled : SAADC_CH_CONFIG_BURST_Disabled)
                                         << SAADC_CH_CONFIG_BURST_Pos)  & SAADC_CH_CONFIG_BURST_Msk)
#endif
      ;

#if defined(SAADC_BURST_BURST_Msk)
  // The nRF54LM20A moved burst out of CH[n].CONFIG into a peripheral-level
  // register; the feature is the same (take 2^OVERSAMPLE samples per
  // TASKS_SAMPLE and average them), only the location changed.
  NRF_SAADC->BURST = saadcBurst ? SAADC_BURST_BURST_Enabled
                                : SAADC_BURST_BURST_Disabled;
#endif

  // Single-ended: negative input is NC, positive input is the caller's pselp.
  NRF_SAADC->CH[0].PSELN = (SAADC_CH_PSELP_CONNECT_NC << SAADC_CH_PSELP_CONNECT_Pos);
  NRF_SAADC->CH[0].PSELP = pselp;

  NRF_SAADC->RESULT.PTR    = (uint32_t)&value;
  // On nRF54L SAADC RESULT.MAXCNT is in BYTES (not samples like nRF52).
  NRF_SAADC->RESULT.MAXCNT = sizeof(value);

  NRF_SAADC->TASKS_START = 1;
  while (!NRF_SAADC->EVENTS_STARTED) ;
  NRF_SAADC->EVENTS_STARTED = 0;

  NRF_SAADC->TASKS_SAMPLE = 1;
  while (!NRF_SAADC->EVENTS_END) ;
  NRF_SAADC->EVENTS_END = 0;

  NRF_SAADC->TASKS_STOP = 1;
  while (!NRF_SAADC->EVENTS_STOPPED) ;
  NRF_SAADC->EVENTS_STOPPED = 0;

  if (value < 0) value = 0;

  NRF_SAADC->ENABLE = (SAADC_ENABLE_ENABLE_Disabled << SAADC_ENABLE_ENABLE_Pos);

  return mapResolution((uint16_t)value, resolution, readResolution);
}

// Build a PSELP value for a physical GPIO. nRF54L SAADC routes any P0/P1/P2
// pin via the same register, encoded as CONNECT | PORT | PIN.
static inline uint32_t pselp_for_gpio(uint32_t gpio)
{
  uint32_t port = gpio >> 5;       // P0=0, P1=1, P2=2
  uint32_t pin  = gpio & 0x1F;
  return ((SAADC_CH_PSELP_CONNECT_AnalogInput << SAADC_CH_PSELP_CONNECT_Pos) & SAADC_CH_PSELP_CONNECT_Msk)
       | ((port << SAADC_CH_PSELP_PORT_Pos) & SAADC_CH_PSELP_PORT_Msk)
       | ((pin  << SAADC_CH_PSELP_PIN_Pos)  & SAADC_CH_PSELP_PIN_Msk);
}

uint32_t analogRead( uint32_t ulPin )
{
  if (ulPin >= PINS_COUNT) return 0;

  uint32_t gpio = g_ADigitalPinMap[ulPin];
  return analogRead_internal(pselp_for_gpio(gpio));
}

uint32_t analogReadVDD( void )
{
  // Internal Vdd rail sense.
  uint32_t pselp =
        ((SAADC_CH_PSELP_CONNECT_Internal << SAADC_CH_PSELP_CONNECT_Pos) & SAADC_CH_PSELP_CONNECT_Msk)
      | ((SAADC_CH_PSELP_INTERNAL_Vdd     << SAADC_CH_PSELP_INTERNAL_Pos) & SAADC_CH_PSELP_INTERNAL_Msk);
  return analogRead_internal(pselp);
}

void analogCalibrateOffset( void )
{
  NRF_SAADC->ENABLE = (SAADC_ENABLE_ENABLE_Enabled << SAADC_ENABLE_ENABLE_Pos);

  NRF_SAADC->EVENTS_CALIBRATEDONE = 0;
  NRF_SAADC->TASKS_CALIBRATEOFFSET = 1;
  while (!NRF_SAADC->EVENTS_CALIBRATEDONE) ;
  NRF_SAADC->EVENTS_CALIBRATEDONE = 0;

  NRF_SAADC->ENABLE = (SAADC_ENABLE_ENABLE_Disabled << SAADC_ENABLE_ENABLE_Pos);
}

#ifdef __cplusplus
}
#endif

#endif /* NRF54L_SERIES */
