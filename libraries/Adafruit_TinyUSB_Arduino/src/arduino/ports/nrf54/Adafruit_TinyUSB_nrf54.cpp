/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach for Adafruit Industries
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "tusb_option.h"

#if defined(ARDUINO_ARCH_NRF54) && CFG_TUD_ENABLED

#include "nrf.h"

#include "Arduino.h"
#include "arduino/Adafruit_USBD_Device.h"

//--------------------------------------------------------------------+
// MACRO TYPEDEF CONSTANT ENUM DECLARATION
//--------------------------------------------------------------------+

#define USBD_STACK_SZ (256)

// The core runs its peripheral interrupts at 3 alongside the SoftDevice
#define USBHS_IRQ_PRIORITY 3

// Bring-up stage, readable over SWD: 1 waiting for VBUS, 2 waiting for
// PCLK24M, 3 starting the stack, 4 running
extern "C" volatile uint32_t tinyusb_nrf54_stage;
volatile uint32_t tinyusb_nrf54_stage = 0;

//--------------------------------------------------------------------+
// Forward USB interrupt events to TinyUSB IRQ Handler
//--------------------------------------------------------------------+
extern "C" void USBHS_IRQHandler(void) { tud_int_handler(0); }

//--------------------------------------------------------------------+
// Porting API
//--------------------------------------------------------------------+

// VREGUSB has no documented status register. Like Zephyr's dwc2 quirk for
// the nRF54L, take bit 2 at offset 0x400 as "VBUS present" for the state at
// start-up; VBUSDETECTED covers a cable plugged in later.
static bool vbus_present(void) {
  uint32_t const status =
      *(volatile uint32_t const *)((uintptr_t)NRF_VREGUSB + 0x400);
  return (status & (1UL << 2)) != 0;
}

// USB Device Driver task
// This top level thread process all usb events and invoke callbacks
static void usb_device_task(void *param) {
  (void)param;

  NVIC_SetPriority(USBHS_IRQn, USBHS_IRQ_PRIORITY);

  // The DWC2 registers can only be accessed once VBUS is valid and PCLK24M
  // runs, so the stack starts only after both
  tinyusb_nrf54_stage = 1;
  NRF_VREGUSB->EVENTS_VBUSDETECTED = 0;
  NRF_VREGUSB->TASKS_START = 1;
  while (!vbus_present() && !NRF_VREGUSB->EVENTS_VBUSDETECTED) {
    vTaskDelay(pdMS_TO_TICKS(100));
  }
  NRF_VREGUSB->EVENTS_VBUSDETECTED = 0;

  // XO24MSTART adds a request for the HFXO next to the SoftDevice's own
  tinyusb_nrf54_stage = 2;
  NRF_CLOCK->EVENTS_XO24MSTARTED = 0;
  NRF_CLOCK->TASKS_XO24MSTART = 1;
  while (!NRF_CLOCK->EVENTS_XO24MSTARTED) {
    vTaskDelay(1);
  }

  tinyusb_nrf54_stage = 3;
  const tusb_rhport_init_t rh_init = {
      .role = TUSB_ROLE_DEVICE,
      .speed = TUD_OPT_HIGH_SPEED ? TUSB_SPEED_HIGH : TUSB_SPEED_FULL,
  };
  tusb_init(0, &rh_init);

  tinyusb_nrf54_stage = 4;

  // RTOS forever loop
  while (1) {
    tud_task();
    TinyUSB_Device_FlushCDC();
  }
}

void TinyUSB_Port_InitDevice(uint8_t rhport) {
  (void)rhport;

  // Create a task for tinyusb device stack
  xTaskCreate(usb_device_task, "usbd", USBD_STACK_SZ, NULL, TASK_PRIO_HIGH,
              NULL);
}

void TinyUSB_Port_EnterDFU(void) {
  // Reset to Bootloader
  enterSerialDfu();
}

uint8_t TinyUSB_Port_GetSerialNumber(uint8_t serial_id[16]) {
  uint32_t *serial_32 = (uint32_t *)serial_id;

  serial_32[0] = __builtin_bswap32(NRF_FICR->INFO.DEVICEID[1]);
  serial_32[1] = __builtin_bswap32(NRF_FICR->INFO.DEVICEID[0]);

  return 8;
}

#endif // ARDUINO_ARCH_NRF54 && CFG_TUD_ENABLED
