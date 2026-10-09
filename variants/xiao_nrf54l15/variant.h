/*
  XIAO nRF54L15 variant for Adafruit nRF54L Arduino core

  Pin mapping derived from Seeed Studio XIAO nRF54L15 schematic.
*/

#ifndef _VARIANT_XIAO_NRF54L15_
#define _VARIANT_XIAO_NRF54L15_

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

// Number of pins defined in PinDescription array
#define PINS_COUNT           (27)
#define NUM_DIGITAL_PINS     (27)
#define NUM_ANALOG_INPUTS    (8)
#define NUM_ANALOG_OUTPUTS   (0)

/*
 * LEDs
 * LED_BUILTIN: P2.00 (active low)
 */
#define PIN_LED1             (16)  // P2.00

#define LED_BUILTIN          PIN_LED1
#define LED_RED              PIN_LED1
#define LED_STATE_ON         0

/*
 * Button
 */
#define PIN_BUTTON1          (17)  // P0.00

/*
 * Analog pins
 *
 * SAADC AIN0..AIN7 are fixed to P1.04-P1.07 and P1.11-P1.14 on the nRF54L15
 * (nrfx SAADC_EXTERNAL_AIN_PSELS; Seeed's pinout marks D0..D3 analog and
 * P1.14 AIN7_VBAT). A0..A3 are D0..D3, but D4 (P1.10) and the P2 pins D6/D7
 * have no analog input, so A4..A7 are not D4..D7:
 *   A4 = D5 (P1.11), A5/A6 = P1.12/P1.13 (the Sense's PDM microphone clock
 *   and data, not on the header), A7 = P1.14 (the VBAT divider).
 */
#define PIN_A0               (0)   // D0 = P1.04 = AIN0
#define PIN_A1               (1)   // D1 = P1.05 = AIN1
#define PIN_A2               (2)   // D2 = P1.06 = AIN2
#define PIN_A3               (3)   // D3 = P1.07 = AIN3
#define PIN_A4               (5)   // D5 = P1.11 = AIN4
#define PIN_A5               (25)  // P1.12 = AIN5 (PDM_CLK)
#define PIN_A6               (26)  // P1.13 = AIN6 (PDM_DIN)
#define PIN_A7               (24)  // P1.14 = AIN7 (VBAT_READ)

static const uint8_t A0  = PIN_A0;
static const uint8_t A1  = PIN_A1;
static const uint8_t A2  = PIN_A2;
static const uint8_t A3  = PIN_A3;
static const uint8_t A4  = PIN_A4;
static const uint8_t A5  = PIN_A5;
static const uint8_t A6  = PIN_A6;
static const uint8_t A7  = PIN_A7;
#define ADC_RESOLUTION    14

#define PIN_AREF           (0xff)

/*
 * Serial interfaces
 */

// Serial1 — UARTE00 on D6 (TX) / D7 (RX). SERIAL00 is also the SPIM00
// behind the default SPI on D8..D10, so Serial1 and SPI cannot run together.
#define PIN_SERIAL1_TX      (6)   // P2.08
#define PIN_SERIAL1_RX      (7)   // P2.07

// Serial2 is the UART wired to the onboard SAMD11 USB-serial bridge (the
// Zephyr board's console, uart20), so it -- not Serial1 -- is what a host
// sees when the board is plugged in. The core's default UARTE20 reaches P1,
// and nothing else in this variant uses SERIAL20 (Wire is on TWIM22).
#define PIN_SERIAL2_TX      PIN_SAMD11_RX  // P1.09, nRF TX -> bridge RX
#define PIN_SERIAL2_RX      PIN_SAMD11_TX  // P1.08, bridge TX -> nRF RX

/*
 * Map the plain `Serial` symbol (and SERIAL_PORT_MONITOR) onto Serial2 rather
 * than the core's default of Serial1. Serial1 is D6/D7, which is not connected
 * to the USB bridge and shares SERIAL00 with SPI. Serial1 stays available by
 * name for anyone who does want D6/D7.
 */
#define SERIAL_PORT_CONSOLE  Serial2

/*
 * SPI Interfaces
 */
#define SPI_INTERFACES_COUNT 1

#define PIN_SPI_MOSI         (10)  // D10 = P2.02
#define PIN_SPI_MISO         (9)   // D9 = P2.04
#define PIN_SPI_SCK          (8)   // D8 = P2.01

static const uint8_t SS   = 2;     // D2 = P1.06
static const uint8_t MOSI = PIN_SPI_MOSI;
static const uint8_t MISO = PIN_SPI_MISO;
static const uint8_t SCK  = PIN_SPI_SCK;

/*
 * Wire Interfaces
 *
 * Route Wire to TWIM22, a dedicated TWI controller, instead of the default
 * NRF_TWIM0 alias which lands on SERIAL20 (shared with SPIM20/UARTE20). This
 * matches the NRF54L15-Clean-Arduino-core XIAO convention and keeps I2C free
 * of the SERIALx fabric used by SPI/Serial1 on the P2 header pins.
 */
#define WIRE_INTERFACES_COUNT 1

#define PIN_WIRE_SDA         (4)   // D4 = P1.10
#define PIN_WIRE_SCL         (5)   // D5 = P1.11

#define WIRE_TWIM            NRF_TWIM22
#define WIRE_TWIS            NRF_TWIS22
#define WIRE_IRQN            SERIAL22_IRQn
#define WIRE_IRQ_HANDLER     SERIAL22_IRQHandler

/*
 * Board control pins
 */
#define PIN_SAMD11_RX        (18)  // P1.09 — SAMD11 USB bridge RX (nRF TX)
#define PIN_SAMD11_TX        (19)  // P1.08 — SAMD11 USB bridge TX (nRF RX)
#define PIN_IMU_MIC_PWR      (20)  // P0.01 — Sense IMU+MIC power enable
#define PIN_RF_SW            (21)  // P2.03 — RF switch power enable
#define PIN_RF_SW_CTL        (22)  // P2.05 — RF path select (0=ceramic, 1=ext)
#define PIN_VBAT_EN          (23)  // P1.15 — VBAT divider enable

/*
 * VBAT sense: P1.14 (AIN7) behind a 1:2 resistor divider that PIN_VBAT_EN
 * switches on (drive it HIGH and let the divider settle before reading).
 * The divider is a high-impedance source, so give the SAADC the longest
 * acquisition time with analogSampleTime(40) first.
 * The pin sees half the battery voltage, so scale the reading by 2, as
 * Seeed's XIAO nRF54L15 battery example does:
 *   mV = analogRead(PIN_VBAT_READ) * 3600 / 4096 * 2   (12-bit, AR_DEFAULT)
 */
#define PIN_VBAT_READ        (24)  // P1.14 — VBAT divider sense (AIN7)
#define PIN_PDM_CLK          (25)  // P1.12 — Sense PDM microphone clock (AIN5)
#define PIN_PDM_DIN          (26)  // P1.13 — Sense PDM microphone data (AIN6)

#ifdef __cplusplus
}
#endif

/*----------------------------------------------------------------------------
 *        Arduino objects - C++ only
 *----------------------------------------------------------------------------*/

#endif
