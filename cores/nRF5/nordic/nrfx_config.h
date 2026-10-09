/*
 * nrfx configuration for the nRF54L Arduino framework.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Drivers are ENABLED=1 unless they're known to fail compilation on
 * nRF54L. Test-driven: we only disable a driver after the build flags
 * a specific error. The Adafruit-derived core mostly uses register-
 * direct HAL inlines and doesn't actually call into the nrfx driver
 * API, but compiling the driver sources is harmless (the linker
 * eliminates unused functions via --gc-sections) and lets the upstream
 * driver code stay one git pull away.
 *
 * Each NRFX_<X>_ENABLED define has two effects:
 *   1. Internal nrfx-driver feature paths that test
 *      NRFX_CHECK(NRFX_<X>_ENABLED) compile out when 0.
 *   2. The file-level guard at the top of each driver .c gates the
 *      whole translation unit on the same macro.
 */

#ifndef NRFX_CONFIG_H__
#define NRFX_CONFIG_H__

// Default IRQ priority for nrfx drivers (Cortex-M33 NVIC: 0 highest, 7 lowest)
#define NRFX_DEFAULT_IRQ_PRIORITY  7

//------------------------------------------------------------------
// Drivers explicitly disabled (with reason)
//
// Add to this list only when the build flags a concrete error for the
// driver - this is the "broken on nRF54L" set, not the "we don't link
// against it" set.
//------------------------------------------------------------------

// nrfx_adc references NRF_ADC / ADC_IRQn / NRF_ADC_Type which is the
// nRF51-era legacy ADC. nRF54L only has SAADC.
#define NRFX_ADC_ENABLED                            0

// nrfx_bellboard references NRF_BELLBOARD which is an inter-core IPC
// peripheral exclusive to nRF54H. Also expanding NRFX_OFFSETOF against
// the missing NRF_BELLBOARD_Type was the source of the cryptic
// nrfx_utils.h:517 "expected expression before ')'".
#define NRFX_BELLBOARD_ENABLED                      0

// nrfx_clock_lfclk requires NRFX_CLOCK_CONFIG_LF_SRC to be defined
// (LFCLK source: RC vs XTAL vs synthesized). The framework already
// drives LFCLK from cores/nRF5/wiring.c via direct register access,
// so we don't need this driver.
#define NRFX_CLOCK_LFCLK_ENABLED                    0

// nrfx_grtc needs NRFX_GRTC_CONFIG_NUM_OF_CC_CHANNELS and the
// NRFX_GRTC_CONFIG_ALLOWED_CC_CHANNELS_MASK to be defined - which CC
// channels are reserved for SoftDevice vs available to the application
// is deployment-specific. Our FreeRTOS port talks to GRTC registers
// directly (cores/nRF5/freertos/portable/CMSIS/nrf54l/), so we don't
// need this driver.
#define NRFX_GRTC_ENABLED                           0

// nrfx_gpiote's "FOREACH_INDEXED_PRESENT" expansion expects per-instance
// NRFX_GPIOTE<N>_ENABLED defines to match the GPIOTE_COUNT array size;
// without them the static initializer overruns the available_channels_mask
// array. Our WInterrupts.c uses HAL inlines (nrf_gpiote_int_*) directly,
// so we don't need this driver.
#define NRFX_GPIOTE_ENABLED                         0

// nrfx_ipc needs NRF_IPC / IPC_IRQn / IPC_CONF_NUM. nRF54L has no IPC
// peripheral - inter-core signalling on nRF54H uses BELLBOARD (already
// disabled), and nRF54L is single-core so neither applies.
#define NRFX_IPC_ENABLED                            0

// nrfx_mramc references NRF_MRAMC and NRF_FICR_INFO_Type.MRAM. nRF54L
// has no MRAM - non-volatile storage is RRAMC. (The cryptic
// nrfx_utils.h:517 errors during the run-25792045323 build were also
// downstream of this header chain.)
#define NRFX_MRAMC_ENABLED                          0

// nrfx_nvmc references NRF_NVMC / NRF_NVMC_Type / NVMC_CONFIG_WEN_* /
// NVMC_FLASH_BASE_ADDRESS. nRF54L's non-volatile memory is RRAM, not
// Flash, and is driven through the RRAMC peripheral (or via the
// SoftDevice's sd_flash_* SVC calls for app code). The legacy NVMC
// peripheral simply does not exist.
#define NRFX_NVMC_ENABLED                           0

// nrfx_qspi needs NRF_QSPI / QSPI_IRQn. QSPI was an nRF52840-only
// peripheral; nRF54L drives external flash through a regular SPIM
// instance (one of the SERIAL slots).
#define NRFX_QSPI_ENABLED                           0

// nrfx_rng needs NRF_RNG / RNG_IRQn. nRF54L has no standalone RNG -
// hardware randomness comes from the CRACEN TRNG (NRFX_CRACEN_ENABLED
// stays on, CRACEN_IRQHandler exists on the application core).
#define NRFX_RNG_ENABLED                            0

// nrfx_rtc needs the legacy RTC peripheral. nRF54L has no plain
// RTC_IRQHandler; time-of-day work is supposed to go through GRTC
// instead (GRTC_0..3_IRQHandler). Disabling this also clears the
// downstream nrfx_utils.h:517 "expected expression before ')'"
// cascade that the empty macro arg from the RTC failure was producing.
#define NRFX_RTC_ENABLED                            0

// nrfx_spi is the legacy non-DMA SPI driver. On nRF54L the SPI master
// lives in the SERIAL slots and is DMA-based; use NRFX_SPIM instead
// (which stays enabled). The driver also #errors out with "No enabled
// SPI instances" because no NRFX_SPI<n>_ENABLED is set.
#define NRFX_SPI_ENABLED                            0

// nrfx_tbm is the Trace Buffer Monitor driver - an nRF53-era debug
// aid for ETM trace capture. Not present on nRF54L.
#define NRFX_TBM_ENABLED                            0

// nrfx_tdm targets a Time-Division-Multiplexed audio interface that
// only ships on nRF54H/nRF53 audio variants. nRF54L audio is I2S + PDM
// (both stay enabled).
#define NRFX_TDM_ENABLED                            0

// nrfx_twi is the legacy non-DMA I2C driver. nRF54L's I2C masters live
// in the SERIAL slots and are DMA-based, accessed through nrfx_twim
// (NRFX_TWIM_ENABLED stays on). The driver was also #erroring out with
// "No enabled TWI instances" because no NRFX_TWI<n>_ENABLED is set.
#define NRFX_TWI_ENABLED                            0

// TWIS is present on nRF54L (one of the SERIAL slots in I2C-slave mode)
// but the driver's #if checks need these two macros to resolve. Defaults
// keep the driver doing its own init and skip the extra sync mode.
#define NRFX_TWIS_NO_SYNC_MODE                      1
#define NRFX_TWIS_ASSUME_INIT_AFTER_RESET_ONLY      0

// nrfx_uart is the legacy non-DMA UART driver. nRF54L's UARTs are in
// the SERIAL slots and DMA-based via nrfx_uarte (stays on).
#define NRFX_UART_ENABLED                           0

// nRF54L has no USB peripheral. NRF_USBD / NRF_USBREG and the matching
// IRQs don't exist on this silicon. (BLE is the only radio link;
// host-side serial on the XIAO boards goes through the SAMD11 bridge.)
#define NRFX_USBD_ENABLED                           0
#define NRFX_USBREG_ENABLED                         0

// VEVIF is the VPR Event Interface - a VPR-coprocessor-side driver
// for signalling from VPR to the application core. The app-core build
// sees VPR00_IRQHandler (the IRQ raised by the VPR) but not the
// VPRCLIC_*_IRQn vectors the driver references. nrfx_vevif only
// compiles when targeting the FLPR/VPR variant of the MDK headers.
#define NRFX_VEVIF_ENABLED                          0

// IPCT (Inter-Processor Communication Transmitter, distinct from the
// older IPC) is an nRF54H thing for DPPI cross-domain routing. Not in
// nRF54L15's _PRESENT 1 list (only SPU*_IPCT 0 placeholders showing
// zero instances per SPU domain). nrfx_gppiv1_ipct.c #errors with
// "Platform not supported" if DPPI_TYPE_IPCT isn't defined and is
// gated on this macro via a wrapper in that file.
#define NRFX_IPCT_ENABLED                           0

// NFCT exists on nRF54L; the driver just needs to be told which TIMER
// instance to use for its anti-collision timing. The macro is
// concatenated into NRF_TIMER<id> verbatim, and on nRF54L the TIMER
// instances are two-digit (TIMER00, TIMER10, TIMER20..24) - use 00,
// not 0, or you get NRF_TIMER0 which doesn't exist.
#define NRFX_NFCT_CONFIG_TIMER_INSTANCE_ID          00

//------------------------------------------------------------------
// Driver IRQ priorities (used when ENABLED=1, harmless otherwise)
//------------------------------------------------------------------

// POWER and CLOCK share an NVIC line on nRF54L (CLOCK_POWER_IRQHandler);
// nrfx_power_clock.h enforces equal priorities and #errors out otherwise.
#define NRFX_POWER_DEFAULT_CONFIG_IRQ_PRIORITY      7
#define NRFX_CLOCK_DEFAULT_CONFIG_IRQ_PRIORITY      7
#define NRFX_TEMP_DEFAULT_CONFIG_IRQ_PRIORITY       7
// TIMER exists on nRF54L (TIMER00..24); nrfx_timer.c expects a default
// IRQ priority and doesn't fall back to NRFX_DEFAULT_IRQ_PRIORITY.
#define NRFX_TIMER_DEFAULT_CONFIG_IRQ_PRIORITY      7

//------------------------------------------------------------------
// Everything else: ENABLED=1 by default. The upstream NRFX_CHECK
// treats undefined macros as 0, so we have to spell out =1 for any
// driver we want compiled.
//------------------------------------------------------------------

#define NRFX_CLOCK_ENABLED                          1
#define NRFX_CLOCK_HFCLK_ENABLED                    1
#define NRFX_CLOCK_HFCLK192M_ENABLED                1
#define NRFX_CLOCK_HFCLKAUDIO_ENABLED               1
#define NRFX_CLOCK_XO_ENABLED                       1
#define NRFX_CLOCK_XO24M_ENABLED                    1
#define NRFX_COMP_ENABLED                           1
#define NRFX_CRACEN_ENABLED                         1
#define NRFX_DPPI_ENABLED                           1
#define NRFX_EGU_ENABLED                            1
// I2S exists on nRF54L05/L10/L15 but not on nRF54LM20A: its MDK headers
// declare no NRF_I2S_Type and no I2S20_IRQn, so nrf_i2s.h fails to compile
// ("unknown type name 'NRF_I2S_Type'"). Keep the driver on where the
// peripheral actually exists.
#if defined(NRF54LM20A_XXAA)
#define NRFX_I2S_ENABLED                            0
#else
#define NRFX_I2S_ENABLED                            1
#endif
#define NRFX_KMU_ENABLED                            1
#define NRFX_LPCOMP_ENABLED                         1
#define NRFX_NFCT_ENABLED                           1
#define NRFX_PDM_ENABLED                            1
#define NRFX_POWER_ENABLED                          1
#define NRFX_PWM_ENABLED                            1
#define NRFX_QDEC_ENABLED                           1
#define NRFX_RRAMC_ENABLED                          1
#define NRFX_RRAMC_DEFAULT_CONFIG_IRQ_PRIORITY      7
#define NRFX_SAADC_ENABLED                          1
#define NRFX_SPIM_ENABLED                           1
#define NRFX_SPIS_ENABLED                           1
#define NRFX_SYSTICK_ENABLED                        1
#define NRFX_TEMP_ENABLED                           1
#define NRFX_TIMER_ENABLED                          1
#define NRFX_TWIM_ENABLED                           1
#define NRFX_TWIS_ENABLED                           1
#define NRFX_UARTE_ENABLED                          1
#define NRFX_WDT_ENABLED                            1

#endif // NRFX_CONFIG_H__
