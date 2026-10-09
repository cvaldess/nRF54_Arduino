/*
  XIAO nRF54LM20A variant for Adafruit nRF54L Arduino core

  Pin mapping derived from the Seeed Studio XIAO nRF54LM20A board definition
  in nrfconnect/sdk-zephyr (boards/seeed/xiao_nrf54lm20a).
*/

#ifndef _VARIANT_XIAO_NRF54LM20A_
#define _VARIANT_XIAO_NRF54LM20A_

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
#define PINS_COUNT           (47)
#define NUM_DIGITAL_PINS     (47)
#define NUM_ANALOG_INPUTS    (8)
#define NUM_ANALOG_OUTPUTS   (0)

/*
 * LEDs
 *
 * On-board RGB LED. Zephyr's xiao_nrf54lm20a-common.dtsi has red on P1.22,
 * blue on P1.23 and green on P1.24, all three GPIO_ACTIVE_LOW, which is also
 * what nRF54_Bootloader's src/boards/xiao_nrf54lm20a/board.h uses.
 */
#define PIN_LED1             (28)  // P1.22 red
#define PIN_LED2             (29)  // P1.23 blue
#define PIN_LED3             (30)  // P1.24 green

#define LED_BUILTIN          PIN_LED1
#define LED_RED              PIN_LED1
#define LED_BLUE             PIN_LED2
#define LED_GREEN            PIN_LED3
#define LED_STATE_ON         0

/*
 * Button
 */
#define PIN_BUTTON1          (31)  // P0.09 (active low, external pull-up)

/*
 * Analog pins
 *
 * SAADC AIN0..AIN7 land on P1.00/31/30/29/06/05/04/03, which on this board
 * are D0, D1, D2, D3, D10, D9, D8 and D4. A4..A7 are therefore NOT D4..D7 --
 * the XIAO nRF54L15's straight A0..A7 = D0..D7 mapping does not carry over.
 */
#define PIN_A0               (0)   // D0  = P1.00 = AIN0
#define PIN_A1               (1)   // D1  = P1.31 = AIN1
#define PIN_A2               (2)   // D2  = P1.30 = AIN2
#define PIN_A3               (3)   // D3  = P1.29 = AIN3
#define PIN_A4               (10)  // D10 = P1.06 = AIN4
#define PIN_A5               (9)   // D9  = P1.05 = AIN5
#define PIN_A6               (8)   // D8  = P1.04 = AIN6
#define PIN_A7               (4)   // D4  = P1.03 = AIN7

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
 *
 * Serial1 runs on UARTE21, not the core's default UARTE00. SERIAL00 can only
 * reach P2, and every P2 pin on this board belongs to the external SPI flash,
 * so UARTE00 has nowhere to go. UARTE21 reaches D6/D7 on P1.
 */
#define PIN_SERIAL1_TX      (6)   // D6 = P1.08
#define PIN_SERIAL1_RX      (7)   // D7 = P1.09

#define SERIAL1_UARTE        NRF_UARTE21
#define SERIAL1_IRQN         SERIAL21_IRQn
#define SERIAL1_IRQ_HANDLER  SERIAL21_IRQHandler

// Serial2 is the board's debug console header (not brought out to D pins).
// It is the UART wired to the onboard SAMD11 USB-serial bridge, so it -- not
// Serial1 -- is what a host sees when the board is plugged in.
#define PIN_SERIAL2_TX      (39)  // P1.11
#define PIN_SERIAL2_RX      (40)  // P1.10

/*
 * Map the plain `Serial` symbol (and SERIAL_PORT_MONITOR) onto Serial2 rather
 * than the core's default of Serial1. Serial1 is D6/D7, which is not connected
 * to the USB bridge, and on the MMM carrier D6/D7 are the I2C bus, so Serial1
 * is unusable there. Sketches that print to `Serial` want the USB bridge.
 * Serial1 stays available by name for anyone who does want D6/D7.
 */
#define SERIAL_PORT_CONSOLE  Serial2

/*
 * SPI Interfaces
 *
 * SPIM23 rather than the default SPIM00, for the same reason as Serial1:
 * the header SPI pins are on P1 and SPIM00 only reaches P2.
 */
#define SPI_INTERFACES_COUNT 1

#define PIN_SPI_SCK          (8)   // D8  = P1.04
#define PIN_SPI_MISO         (9)   // D9  = P1.05
#define PIN_SPI_MOSI         (10)  // D10 = P1.06

#define SPI_SPIM             NRF_SPIM23

static const uint8_t SS   = 2;     // D2 = P1.30
static const uint8_t MOSI = PIN_SPI_MOSI;
static const uint8_t MISO = PIN_SPI_MISO;
static const uint8_t SCK  = PIN_SPI_SCK;

/*
 * Wire Interfaces
 *
 * TWIM22 is the XIAO I2C controller on this board (D4/D5). Same dedicated-TWI
 * reasoning as the XIAO nRF54L15 variant.
 */
#define WIRE_INTERFACES_COUNT 2

#define PIN_WIRE_SDA         (4)   // D4 = P1.03
#define PIN_WIRE_SCL         (5)   // D5 = P1.07

#define WIRE_TWIM            NRF_TWIM22
#define WIRE_TWIS            NRF_TWIS22
#define WIRE_IRQN            SERIAL22_IRQn
#define WIRE_IRQ_HANDLER     SERIAL22_IRQHandler

/*
 * Wire1 is the private bus to the on-board nPM1300 PMIC (P1.18/P1.17). It is
 * not brought out to the header, so nothing else can appear on it.
 *
 * TWIM24 is the only SERIAL2x instance this board leaves free -- SERIAL20 is
 * the console UART, 21 is Serial1, 22 is Wire and 23 is SPI. It has to be a
 * SERIAL2x: SERIAL00 reaches only P2 and SERIAL30 only P0, while the PMIC
 * pins are on P1.
 */
#define PIN_WIRE1_SDA        PIN_PMIC_SDA
#define PIN_WIRE1_SCL        PIN_PMIC_SCL

#define WIRE1_TWIM           NRF_TWIM24
#define WIRE1_TWIS           NRF_TWIS24
#define WIRE1_IRQN           SERIAL24_IRQn
#define WIRE1_IRQ_HANDLER    SERIAL24_IRQHandler

/*
 * Board control pins
 */
#define PIN_POWER_EN         (32)  // P1.12 — board power enable (regulator-boot-on)
#define PIN_PDM_CLK          (33)  // P1.13 — on-board DMIC clock
#define PIN_PDM_DIN          (34)  // P1.14 — on-board DMIC data
#define PIN_PMIC_SDA         (35)  // P1.18 — nPM1300 PMIC (Wire1 / TWIM24)
#define PIN_PMIC_SCL         (36)  // P1.17 — nPM1300 PMIC (Wire1 / TWIM24)
#define PIN_IMU_SDA          (37)  // P0.08 — LSM6DS3TR-C on TWIM30
#define PIN_IMU_SCL          (38)  // P0.07 — LSM6DS3TR-C on TWIM30

/* External SPI flash (PY25Q64HA) on SPIM00. Named so a sketch can hold the
 * part deselected; the core does not drive it. */
#define PIN_QSPI_SCK         (41)  // P2.01
#define PIN_QSPI_MOSI        (42)  // P2.02
#define PIN_QSPI_MISO        (43)  // P2.04
#define PIN_QSPI_CS          (44)  // P2.05
#define PIN_QSPI_HOLD        (45)  // P2.00
#define PIN_QSPI_WP          (46)  // P2.03

#ifdef __cplusplus
}
#endif

/*----------------------------------------------------------------------------
 *        Arduino objects - C++ only
 *----------------------------------------------------------------------------*/

#endif
