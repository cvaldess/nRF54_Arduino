/*
  XIAO nRF54LM20A variant for Adafruit nRF54L Arduino core

  Pin mapping derived from the Seeed Studio XIAO nRF54LM20A board definition
  in nrfconnect/sdk-zephyr (boards/seeed/xiao_nrf54lm20a).
*/

#include "variant.h"
#include "wiring_constants.h"
#include "wiring_digital.h"
#include "nrf.h"

/*
 * Pin mapping: Arduino pin index → physical GPIO number
 *
 * Physical GPIO = port * 32 + pin_within_port
 * P0.xx = 0..31, P1.xx = 32..63, P2.xx = 64..95, P3.xx = 96..127
 *
 * The nRF54LM20A has a fourth GPIO port (P3) that the nRF54L15 does not,
 * and the XIAO header spans P0, P1 and P3.
 */
const uint32_t g_ADigitalPinMap[] =
{
  // D0..D10: primary header (P1 port)
  32,   // D0  = P1.00 (A0)
  63,   // D1  = P1.31 (A1)
  62,   // D2  = P1.30 (A2)
  61,   // D3  = P1.29 (A3)
  35,   // D4  = P1.03 (SDA, A7)
  39,   // D5  = P1.07 (SCL)
  40,   // D6  = P1.08 (TX)
  41,   // D7  = P1.09 (RX)
  36,   // D8  = P1.04 (SCK, A6)
  37,   // D9  = P1.05 (MISO, A5)
  38,   // D10 = P1.06 (MOSI, A4)

  // D11..D18: extended pads (P3 port)
  96,   // D11 = P3.00
  97,   // D12 = P3.01
  98,   // D13 = P3.02
  99,   // D14 = P3.03
  100,  // D15 = P3.04
  101,  // D16 = P3.05
  102,  // D17 = P3.06
  103,  // D18 = P3.07

  // D19..D24: extended pads (P0 port)
  0,    // D19 = P0.00
  1,    // D20 = P0.01
  2,    // D21 = P0.02
  3,    // D22 = P0.03
  4,    // D23 = P0.04
  5,    // D24 = P0.05

  // D25..D27: extended pads (P3 port)
  105,  // D25 = P3.09
  106,  // D26 = P3.10
  107,  // D27 = P3.11

  // Internal pins
  54,   // 28 = LED_RED = P1.22
  55,   // 29 = LED_BLUE = P1.23
  56,   // 30 = LED_GREEN = P1.24
  9,    // 31 = BUTTON = P0.09
  44,   // 32 = POWER_EN = P1.12
  45,   // 33 = PDM_CLK = P1.13
  46,   // 34 = PDM_DIN = P1.14
  50,   // 35 = PMIC_SDA = P1.18
  49,   // 36 = PMIC_SCL = P1.17
  8,    // 37 = IMU_SDA = P0.08
  7,    // 38 = IMU_SCL = P0.07
  43,   // 39 = SERIAL2_TX (console) = P1.11
  42,   // 40 = SERIAL2_RX (console) = P1.10

  // External SPI flash (PY25Q64HA) on SPIM00
  65,   // 41 = QSPI_SCK = P2.01
  66,   // 42 = QSPI_MOSI = P2.02
  68,   // 43 = QSPI_MISO = P2.04
  69,   // 44 = QSPI_CS = P2.05
  64,   // 45 = QSPI_HOLD = P2.00
  67,   // 46 = QSPI_WP = P2.03
};


void initVariant()
{
  // RGB LED off (active low)
  pinMode(PIN_LED1, OUTPUT);
  ledOff(PIN_LED1);
  pinMode(PIN_LED2, OUTPUT);
  ledOff(PIN_LED2);
  pinMode(PIN_LED3, OUTPUT);
  ledOff(PIN_LED3);

  // The board's sensor/flash rail is gated; the Zephyr board marks this
  // regulator boot-on, so bring it up before anything tries to talk to the
  // IMU, DMIC or external flash.
  pinMode(PIN_POWER_EN, OUTPUT);
  digitalWrite(PIN_POWER_EN, HIGH);

  // Leave the external flash deselected so it does not drive MISO.
  pinMode(PIN_QSPI_CS, OUTPUT);
  digitalWrite(PIN_QSPI_CS, HIGH);
}
