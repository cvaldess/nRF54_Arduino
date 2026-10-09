/*
  Copyright (c) 2014-2015 Arduino LLC.  All right reserved.
  Copyright (c) 2016 Sandeep Mistry All right reserved.
  Copyright (c) 2018, Adafruit Industries (adafruit.com)

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

#ifndef _VARIANT_NRF54LM20DK_
#define _VARIANT_NRF54LM20DK_

/** Master clock frequency */
#define VARIANT_MCK       (128000000ul)

#define USE_LFXO      // Board uses 32khz crystal for LF

/*----------------------------------------------------------------------------
 *        Headers
 *----------------------------------------------------------------------------*/

#include "WVariant.h"

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

/*
 * nRF54LM20 DK (PCA10184) Pin Map: Arduino pin = physical GPIO number.
 *   P0.n = n, P1.n = 32 + n, P2.n = 64 + n, P3.n = 96 + n
 *   P0 has 10 pins, P1 32, P2 11, P3 13 (MDK P<n>_PIN_NUM_MAX).
 *
 * The DK carries an nRF54LM20B; this core builds for the nRF54LM20A, which is the same die
 * without the Axon NPU (Zephyr's nrf54lm20dk/nrf54lm20a target runs on the same board).
 *
 * Serial peripherals are bound to a GPIO port, as on the nRF54L15:
 *   SERIAL00 (UARTE00/SPIM00) -> P2, SERIAL2x -> P1 and P3, SERIAL30 -> P0.
 * Pin interrupts: P0 -> GPIOTE30 (4 channels), P1 and P3 -> GPIOTE20 (8 channels), P2 none.
 *
 * LEDs, buttons, VCOM and the MX25R64 pins: nRF54LM20 DK User Guide v1.0.2 (2.3-2.5, 3.1.2).
 * Not free by default: P1.01/P1.02 (NFC), P1.20/P1.21 (32.768 kHz crystal), P2.00-P2.05 (MX25R64,
 * switchable to the headers by the board controller), P2.06-P2.10 (trace).
 */

// Number of pins defined in PinDescription array
#define PINS_COUNT           (109)
#define NUM_DIGITAL_PINS     (109)
#define NUM_ANALOG_INPUTS    (8)
#define NUM_ANALOG_OUTPUTS   (0)

// LEDs (active HIGH, unlike the nRF54L15-DK): LED0 P1.22, LED1 P1.25, LED2 P1.27, LED3 P1.28
#define PIN_LED1             (54)
#define PIN_LED2             (57)
#define PIN_LED3             (59)
#define PIN_LED4             (60)

#define LED_BUILTIN          PIN_LED1
#define LED_CONN             PIN_LED2

#define LED_RED              PIN_LED1
#define LED_BLUE             PIN_LED2

#define LED_STATE_ON         1         // State when LED is lit (active high)

// Buttons (active low, need the internal pull-up): BTN0 P1.26, BTN1 P1.09, BTN2 P1.08, BTN3 P0.05
#define PIN_BUTTON1          (58)
#define PIN_BUTTON2          (41)
#define PIN_BUTTON3          (40)
#define PIN_BUTTON4          (5)

// Analog: AIN0..AIN7 = P1.00, P1.31, P1.30, P1.29, P1.06, P1.05, P1.04, P1.03
// (nRF54LM20 datasheet v1.0, CSP98 pin assignments)
#define PIN_A0               (32)
#define PIN_A1               (63)
#define PIN_A2               (62)
#define PIN_A3               (61)
#define PIN_A4               (38)
#define PIN_A5               (37)
#define PIN_A6               (36)
#define PIN_A7               (35)

static const uint8_t A0  = PIN_A0;
static const uint8_t A1  = PIN_A1;
static const uint8_t A2  = PIN_A2;
static const uint8_t A3  = PIN_A3;
static const uint8_t A4  = PIN_A4;
static const uint8_t A5  = PIN_A5;
static const uint8_t A6  = PIN_A6;
static const uint8_t A7  = PIN_A7;
#define ADC_RESOLUTION    14

// Other pins
#define PIN_AREF           (0xff)  // No external AREF on nRF54L

/*
 * Serial interfaces
 *
 * VCOM1 of the on-board J-Link (UARTE30 on P0.06/P0.07) shares SERIAL30 with Wire below, so it
 * is not declared as Serial2.
 */

// Serial1: VCOM0 on the on-board J-Link (UARTE20): TX P1.16, RX P1.17 (RTS P1.18, CTS P1.19 unused)
#define PIN_SERIAL1_RX      (49)
#define PIN_SERIAL1_TX      (48)
#define SERIAL1_UARTE       NRF_UARTE20
#define SERIAL1_IRQN        SERIAL20_IRQn
#define SERIAL1_IRQ_HANDLER SERIAL20_IRQHandler

/*
 * SPI Interfaces (SPIM00, the on-board MX25R64; its pins also reach the expansion header)
 */
#define SPI_INTERFACES_COUNT 1

#define PIN_SPI_MISO         (68)  // P2.04
#define PIN_SPI_MOSI         (66)  // P2.02
#define PIN_SPI_SCK          (65)  // P2.01

static const uint8_t SS   = 69;    // P2.05 (MX25R64 CS)
static const uint8_t MOSI = PIN_SPI_MOSI;
static const uint8_t MISO = PIN_SPI_MISO;
static const uint8_t SCK  = PIN_SPI_SCK;

/*
 * Wire Interfaces (TWIM30 on P0.03/P0.04: header P1 and the expansion header; the DK labels no I2C pins)
 */
#define WIRE_INTERFACES_COUNT 1

#define PIN_WIRE_SDA         (3)   // P0.03
#define PIN_WIRE_SCL         (4)   // P0.04

#define WIRE_TWIM            NRF_TWIM30
#define WIRE_TWIS            NRF_TWIS30
#define WIRE_IRQN            SERIAL30_IRQn
#define WIRE_IRQ_HANDLER     SERIAL30_IRQHandler

#ifdef __cplusplus
}
#endif

/*----------------------------------------------------------------------------
 *        Arduino objects - C++ only
 *----------------------------------------------------------------------------*/

#endif
