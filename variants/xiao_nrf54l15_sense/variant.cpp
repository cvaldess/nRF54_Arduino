/*
  XIAO nRF54L15 variant for Adafruit nRF54L Arduino core

  Pin mapping derived from Seeed Studio XIAO nRF54L15 schematic.
*/

#include "variant.h"
#include "wiring_constants.h"
#include "wiring_digital.h"
#include "nrf.h"

/*
 * Pin mapping: Arduino pin index → physical GPIO number
 *
 * Physical GPIO = port * 32 + pin_within_port
 * P0.xx = 0..31, P1.xx = 32..63, P2.xx = 64..95
 */
const uint32_t g_ADigitalPinMap[] =
{
  // D0..D5: header pins (P1 port)
  36,   // D0  = P1.04
  37,   // D1  = P1.05
  38,   // D2  = P1.06
  39,   // D3  = P1.07
  42,   // D4  = P1.10 (SDA)
  43,   // D5  = P1.11 (SCL, A4)

  // D6..D10: header pins (P2 port)
  72,   // D6  = P2.08 (TX)
  71,   // D7  = P2.07 (RX)
  65,   // D8  = P2.01 (SCK)
  68,   // D9  = P2.04 (MISO)
  66,   // D10 = P2.02 (MOSI)

  // D11..D15: back pads / misc
  3,    // D11 = P0.03
  4,    // D12 = P0.04
  74,   // D13 = P2.10
  73,   // D14 = P2.09
  70,   // D15 = P2.06

  // Internal pins
  64,   // 16 = LED_BUILTIN = P2.00
  0,    // 17 = BUTTON = P0.00
  41,   // 18 = SAMD11_RX = P1.09
  40,   // 19 = SAMD11_TX = P1.08
  1,    // 20 = IMU_MIC_PWR = P0.01
  67,   // 21 = RF_SW = P2.03
  69,   // 22 = RF_SW_CTL = P2.05
  47,   // 23 = VBAT_EN = P1.15
  46,   // 24 = VBAT_READ = P1.14 (AIN7)
  44,   // 25 = PDM_CLK = P1.12 (AIN5)
  45,   // 26 = PDM_DIN = P1.13 (AIN6)
};


void initVariant()
{
  // LED off (active low)
  pinMode(PIN_LED1, OUTPUT);
  ledOff(PIN_LED1);
}
