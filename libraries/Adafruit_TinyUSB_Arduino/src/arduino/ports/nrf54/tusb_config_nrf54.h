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

#ifndef _TUSB_CONFIG_NRF54_H_
#define _TUSB_CONFIG_NRF54_H_

#ifdef __cplusplus
extern "C" {
#endif

// nRF54LM20: the USBHS block is a Synopsys DWC2 core with a high-speed PHY
// (OPT_MCU_NRF54 selects the dwc2 port, see dwc2_nrf.h). Device only.

#ifndef pdTICKS_TO_MS
#define pdTICKS_TO_MS(xTimeInTicks)                                            \
  ((TickType_t)(((uint64_t)(xTimeInTicks) * (uint64_t)1000U) /                 \
                (uint64_t)configTICK_RATE_HZ))
#endif

//--------------------------------------------------------------------
// COMMON CONFIGURATION
//--------------------------------------------------------------------
#define CFG_TUSB_MCU OPT_MCU_NRF54
#define CFG_TUSB_OS OPT_OS_FREERTOS

#ifndef CFG_TUSB_DEBUG
#define CFG_TUSB_DEBUG 0
#endif

#define CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_ALIGN __attribute__((aligned(4)))

#ifdef USE_TINYUSB
#define CFG_TUD_ENABLED 1
#else
#define CFG_TUD_ENABLED 0
#endif
#define CFG_TUH_ENABLED 0

#define CFG_TUD_MAX_SPEED OPT_MODE_HIGH_SPEED

// USB identity. Nothing is registered for this board: keep the library's
// default VID/PID (an Adafruit VID that host tools already list as a serial
// device) and only name the product.
#ifndef USB_MANUFACTURER
#define USB_MANUFACTURER "Nordic Semiconductor"
#endif
#ifndef USB_PRODUCT
#define USB_PRODUCT "nRF54LM20 DK"
#endif

//--------------------------------------------------------------------
// DEVICE CONFIGURATION
//--------------------------------------------------------------------
#if CFG_TUD_ENABLED

#define CFG_TUD_ENDPOINT0_SIZE 64

#ifndef CFG_TUD_CDC
#define CFG_TUD_CDC 1
#endif
#ifndef CFG_TUD_MSC
#define CFG_TUD_MSC 0
#endif
#ifndef CFG_TUD_HID
#define CFG_TUD_HID 0
#endif
#ifndef CFG_TUD_MIDI
#define CFG_TUD_MIDI 0
#endif
#ifndef CFG_TUD_VENDOR
#define CFG_TUD_VENDOR 0
#endif

// Bulk endpoints are 512 bytes at high speed
#define CFG_TUD_CDC_EP_BUFSIZE 512
#define CFG_TUD_CDC_RX_BUFSIZE 512
#define CFG_TUD_CDC_TX_BUFSIZE 1024

#endif // CFG_TUD_ENABLED

#ifdef __cplusplus
}
#endif

#endif // _TUSB_CONFIG_NRF54_H_
