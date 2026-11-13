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

#ifndef _VARIANT_LM20MESHNODE_
#define _VARIANT_LM20MESHNODE_

/** Master clock frequency */
#define VARIANT_MCK       (128000000ul)

#define USE_LFXO      // The ME54BS13 module carries a 32.768 kHz crystal on P1.20/P1.21

/*----------------------------------------------------------------------------
 *        Headers
 *----------------------------------------------------------------------------*/

#include "WVariant.h"

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

/*
 * LM20MeshNode rev 0.1 (https://github.com/cvaldess/LM20MeshNode): Minew ME54BS13 module
 * (nRF54LM20A), EBYTE E22-900MM22S (SX1262), EBYTE E108-GN04S GNSS, nPM1300 PMIC.
 * Arduino pin = physical GPIO number: P0.n = n, P1.n = 32 + n, P2.n = 64 + n, P3.n = 96 + n.
 *
 * Serial peripherals are bound to a GPIO port, as on the nRF54L15:
 *   SERIAL00 (UARTE00/SPIM00) -> P2, SERIAL2x -> P1 and P3, SERIAL30 -> P0.
 * Pin interrupts: P0 -> GPIOTE30 (4 channels), P1 and P3 -> GPIOTE20 (8 channels), P2 none.
 */

// Number of pins defined in PinDescription array
#define PINS_COUNT           (109)
#define NUM_DIGITAL_PINS     (109)
#define NUM_ANALOG_INPUTS    (8)
#define NUM_ANALOG_OUTPUTS   (0)

// Status LED P2.00 (green, active high). The nPM1300 drives the charge and error LEDs.
#define PIN_LED1             (64)
#define LED_BUILTIN          PIN_LED1
#define LED_STATE_ON         1         // State when LED is lit (active high)

// Side buttons to GND (no external pull-ups): OK P1.28, UP P1.27, DOWN P1.25
#define PIN_BUTTON1          (60)
#define PIN_BUTTON2          (59)
#define PIN_BUTTON3          (57)

// Analog: AIN0..AIN7 = P1.00, P1.31, P1.30, P1.29, P1.06, P1.05, P1.04, P1.03
// (nRF54LM20 datasheet v1.0). On this board AIN2 (P1.30) reads the battery through 1M/1M.
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

#define PIN_VBAT           PIN_A2

// Other pins
#define PIN_AREF           (0xff)  // No external AREF on nRF54L

/*
 * Serial interfaces
 */

// Serial1: the E108-GN04S GNSS (UARTE20): nRF TX P1.03 -> GNSS RXD, GNSS TXD -> nRF RX P1.29
#define PIN_SERIAL1_RX      (61)
#define PIN_SERIAL1_TX      (35)
#define SERIAL1_UARTE       NRF_UARTE20
#define SERIAL1_IRQN        SERIAL20_IRQn
#define SERIAL1_IRQ_HANDLER SERIAL20_IRQHandler

/*
 * SPI Interfaces (SPIM22, the E22-900MM22S; P1 runs SPI at 8 MHz at most, SCK on the clock pin P1.17)
 */
#define SPI_INTERFACES_COUNT 1

#define SPI_SPIM             NRF_SPIM22
#define PIN_SPI_MISO         (47)  // P1.15
#define PIN_SPI_MOSI         (48)  // P1.16
#define PIN_SPI_SCK          (49)  // P1.17

static const uint8_t SS   = 50;    // P1.18
static const uint8_t MOSI = PIN_SPI_MOSI;
static const uint8_t MISO = PIN_SPI_MISO;
static const uint8_t SCK  = PIN_SPI_SCK;

/*
 * Wire Interfaces (TWIM30 on P0.03/P0.04, 4.7 kOhm pull-ups on the board): nPM1300 0x6B,
 * LSM6DS3TR-C 0x6A, SHT41 0x44, SSD1306 OLED 0x3C and the Qwiic-style connector J501.
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
