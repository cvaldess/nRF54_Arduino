/*
  Copyright (c) 2015 Arduino LLC.  All right reserved.

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

#include "nrf.h"

#include "delay.h"
#include "Arduino.h"

#ifdef __cplusplus
extern "C" {
#endif

uint32_t millis( void )
{
  return tick2ms(xTaskGetTickCount());
}

void delay( uint32_t ms )
{
  uint32_t ticks = ms2tick(ms);
  vTaskDelay(ticks);
}

void dwt_enable(void)
{
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; /* Global Enable for DWT */
  DWT->CYCCNT = 0;                                /* Reset the counter */
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;            /* Enable cycle counter */
}

void dwt_disable(void)
{
  DWT->CTRL &= ~DWT_CTRL_CYCCNTENA_Msk;
  CoreDebug->DEMCR &= ~CoreDebug_DEMCR_TRCENA_Msk;
}

/* Is the DWT cycle counter actually counting?
 *
 * ARMv8-M leaves DWT_CTRL.CYCCNTENA RAZ/WI when non-invasive debug is not
 * permitted, which is what APPROTECT does. CYCCNT then reads a constant, and a
 * wait for it to advance would never return. Probe it once -- two reads have
 * to differ, because they are at least a cycle apart -- and remember the
 * answer.
 *
 * DEMCR and DWT->CTRL are put back as they were found. Leaving the counter
 * running would silently change what micros() reports: it prefers CYCCNT when
 * the counter is enabled, and CYCCNT wraps every 33 seconds at 128 MHz where
 * the FreeRTOS tick it otherwise uses wraps after weeks. Enabling the cycle
 * counter stays dwt_enable()'s decision to make.
 */
static int8_t _dwt_cyccnt_usable = -1;

static bool dwt_cyccnt_usable(void)
{
  if (_dwt_cyccnt_usable < 0)
  {
    uint32_t core_debug = CoreDebug->DEMCR;
    CoreDebug->DEMCR = core_debug | CoreDebug_DEMCR_TRCENA_Msk;

    uint32_t dwt_ctrl = DWT->CTRL;
    DWT->CTRL = dwt_ctrl | DWT_CTRL_CYCCNTENA_Msk;

    uint32_t first  = DWT->CYCCNT;
    uint32_t second = DWT->CYCCNT;

    _dwt_cyccnt_usable = (second != first) ? 1 : 0;

    DWT->CTRL = dwt_ctrl;
    CoreDebug->DEMCR = core_debug;
  }

  return _dwt_cyccnt_usable > 0;
}

/* Delay by counting cycles rather than by counting loop iterations.
 *
 * nrfx_coredep_delay_us() assumes its inner loop costs
 * NRFX_COREDEP_DELAY_US_LOOP_CYCLES (3) cycles per iteration. On nRF54L,
 * fetching that loop from RRAM costs roughly eight, so the delay overshoots by
 * more than 2x. The DWT cycle counter measures what the core really did, so it
 * cannot be wrong in that way.
 *
 * The cycles-per-microsecond figure comes from SystemCoreClock, which the
 * startup code brings in line with the PLL (128 MHz by default on this
 * family), so nothing here assumes a particular frequency.
 *
 * The fallback stays the nrfx loop. It runs long rather than short, which is
 * the safe direction for a delay, and it is only reached on a part where the
 * cycle counter is unusable. Its loop-cycles constant is deliberately not
 * retuned to match this part; nrfx_glue.h says why.
 */
void nrf54_delay_us(uint32_t us)
{
  if (us == 0)
  {
    return;
  }

  if (!dwt_cyccnt_usable())
  {
    nrfx_coredep_delay_us(us);
    return;
  }

  uint32_t core_debug = CoreDebug->DEMCR;
  CoreDebug->DEMCR = core_debug | CoreDebug_DEMCR_TRCENA_Msk;

  uint32_t dwt_ctrl = DWT->CTRL;
  DWT->CTRL = dwt_ctrl | DWT_CTRL_CYCCNTENA_Msk;

  uint32_t cycles = us * (SystemCoreClock / 1000000UL);
  uint32_t start  = DWT->CYCCNT;

  while ((DWT->CYCCNT - start) < cycles)
  {
  }

  DWT->CTRL = dwt_ctrl;
  CoreDebug->DEMCR = core_debug;
}


#ifdef __cplusplus
}
#endif
