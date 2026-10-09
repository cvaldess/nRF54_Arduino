/*
 * Startup File for nRF54LM20A (Cortex-M33)
 *
 * This file contains:
 * - Interrupt vector table
 * - Reset handler (copies .data, zeroes .bss, calls main)
 * - Default exception/interrupt handlers
 *
 * Licensed under the Apache License 2.0
 */

/* The platform builds all of cores/nRF5 as one library, so every startup
 * file here is assembled for every target. Only one may contribute a
 * vector table. */
#if defined(NRF54LM20A_XXAA)

    .syntax unified
    .cpu cortex-m33
    .thumb
    .fpu fpv5-sp-d16

    .global g_pfnVectors
    .global __isr_vector
    .extern __libc_init_array
    .extern __StackTop

    /* MSP stack (exceptions, SoftDevice); FreeRTOS task stacks come from the heap.
     * nrf54_common.ld sizes __StackLimit from this section and new.cpp's _sbrk()
     * stops the heap there, so without it the heap runs into the stack. */
#ifndef __STACK_SIZE
#define __STACK_SIZE 8192
#endif
    .section .stack
    .align 3
    .space __STACK_SIZE

/* *****************************************************************************
 * Vector Table
 * *****************************************************************************/

    .section .isr_vector, "a"
    .align 4
    .type g_pfnVectors, %object

__isr_vector:
g_pfnVectors:
    .word   __StackTop              /* 0: Initial Stack Pointer */
    .word   Reset_Handler           /* 1: Reset Handler */
    .word   NMI_Handler             /* 2: NMI Handler */
    .word   HardFault_Handler       /* 3: Hard Fault Handler */
    .word   MemManage_Handler       /* 4: MPU Fault Handler */
    .word   BusFault_Handler        /* 5: Bus Fault Handler */
    .word   UsageFault_Handler      /* 6: Usage Fault Handler */
    .word   SecureFault_Handler     /* 7: Secure Fault Handler */
    .word   0                       /* 8: Reserved */
    .word   0                       /* 9: Reserved */
    .word   0                       /* 10: Reserved */
    .word   SVC_Handler             /* 11: SVCall Handler */
    .word   DebugMon_Handler        /* 12: Debug Monitor Handler */
    .word   0                       /* 13: Reserved */
    .word   PendSV_Handler          /* 14: PendSV Handler */
    .word   SysTick_Handler         /* 15: SysTick Handler */

    /* External Interrupts (nRF54LM20A specific).
     * IRQ numbers differ from nRF54L15: the AAR00_CCM00/ECB00/SERIAL00
     * block sits 3-4 lines higher, there is no I2S20, and the table runs
     * to 289 (VREGUSB) rather than 239. */
    /* IRQ 0..27 */
    .rept 28
    .word   Default_Handler
    .endr
    /* IRQ 28..31: SWI00..SWI03. SWI01 is SD_EVT_IRQn (nrf_soc.h), the line the
     * SoftDevice raises to hand the application BLE and SoC events, and
     * bluefruit.cpp defines SD_EVT_IRQHandler for it. Leaving these as
     * Default_Handler put that slot on an infinite loop, so the first
     * SoftDevice event after Bluefruit.begin() hung the device. */
    .word   SWI00_IRQHandler
    .word   SWI01_IRQHandler
    .word   SWI02_IRQHandler
    .word   SWI03_IRQHandler
    /* IRQ 32..73 */
    .rept 42
    .word   Default_Handler
    .endr
    /* IRQ 74..75: AAR00/CCM00 and ECB00. */
    .word   AAR00_CCM00_IRQHandler
    .word   ECB00_IRQHandler
    /* IRQ 76: VPR00 */
    .word   Default_Handler
    /* IRQ 77: SERIAL00 (UARTE00/SPIM00/TWIM00) - Serial1 */
    .word   SERIAL00_IRQHandler
    /* IRQ 78..132 */
    .rept 55
    .word   Default_Handler
    .endr
    /* IRQ 133: TIMER10 - forwarded to the SoftDevice (nordic/sd_isr.S). */
    .word   TIMER10_IRQHandler
    /* IRQ 134..137 */
    .rept 4
    .word   Default_Handler
    .endr
    /* IRQ 138: RADIO_0 - forwarded to the SoftDevice. */
    .word   RADIO_0_IRQHandler
    /* IRQ 139..197 */
    .rept 59
    .word   Default_Handler
    .endr
    /* IRQ 198: SERIAL20 - Serial2, and the legacy SPI0/TWI0 compat alias. */
    .word   SERIAL20_IRQHandler
    /* IRQ 199: SERIAL21 - Serial1 on boards with no free P2 pins. */
    .word   SERIAL21_IRQHandler
    /* IRQ 200: SERIAL22 - Wire target mode on XIAO (WIRE_IRQ_HANDLER). */
    .word   SERIAL22_IRQHandler
    /* IRQ 201..217 */
    .rept 17
    .word   Default_Handler
    .endr
    /* IRQ 218..219: GPIOTE20 IRQ lines used by attachInterrupt(). */
    .word   GPIOTE20_0_IRQHandler
    .word   GPIOTE20_1_IRQHandler
    /* IRQ 220..223 */
    .rept 4
    .word   Default_Handler
    .endr
    /* IRQ 224..225: QDEC20, QDEC21 */
    .word   QDEC20_IRQHandler
    .word   QDEC21_IRQHandler
    /* IRQ 226..228: GRTC interrupt groups. */
    .word   GRTC_0_IRQHandler
    .word   GRTC_1_IRQHandler
    .word   GRTC_2_IRQHandler
    /* IRQ 229: GRTC_3 - the SoftDevice's GRTC group (SD_GRTC_IRQn_USED). The
     * FreeRTOS tick is on GRTC_2, so the two do not overlap. */
    .word   GRTC_3_IRQHandler
    /* IRQ 230..269 */
    .rept 40
    .word   Default_Handler
    .endr
    /* IRQ 270: CLOCK_POWER - forwarded to the SoftDevice. */
    .word   CLOCK_POWER_IRQHandler
    /* IRQ 271..289 */
    .rept 19
    .word   Default_Handler
    .endr

    .size g_pfnVectors, .-g_pfnVectors

/* *****************************************************************************
 * Reset Handler
 * *****************************************************************************/

    .section .text.Reset_Handler
    .global Reset_Handler
    .type Reset_Handler, %function
Reset_Handler:
    /* Disable interrupts */
    cpsid i

    /* Claim the vector table.
     *
     * On nRF52 the MBR lives at address 0 and forwards every exception, so an
     * application entered from a bootloader can leave VTOR alone. nRF54L has
     * no MBR. nRF54_Bootloader's boot stub points VTOR at the bootloader's own
     * table and its app-start path never moves it, so an application that does
     * not claim VTOR here keeps running the bootloader's handlers -- which
     * kills FreeRTOS at the first context switch, because vPortStartFirstTask's
     * SVC and the following PendSV/SysTick dispatch into the wrong image.
     *
     * Note this image's table is near the BOTTOM of RRAM (FLASH origin 0x8000,
     * just above the bootloader) and the SoftDevice sits at the top, the
     * inverse of the nRF52 layout.
     * Do not "correct" this toward nRF52's SoftDevice-at-MBR_SIZE convention.
     *
     * SystemInit is not the place for it: the weak stub at the end of this
     * file is overridden by the MDK's system_nrf54l.c, which never touches
     * VTOR. */
    ldr r0, =__isr_vector
    ldr r1, =0xE000ED08          /* SCB->VTOR */
    str r0, [r1]
    dsb
    isb

    /* Copy .data section from FLASH to RAM. */
    ldr r0, =__etext         /* SRC: .data load address in FLASH */
    ldr r1, =__data_start__  /* DST: .data start in RAM */
    ldr r2, =__data_end__    /* DST: .data end in RAM */

LoopCopyData:
    cmp r1, r2
    bcs LoopCopyDataDone
    ldr r3, [r0], #4
    str r3, [r1], #4
    b LoopCopyData

LoopCopyDataDone:
    /* Zero out .bss section. */
    ldr r0, =__bss_start__
    ldr r1, =__bss_end__
    mov r2, #0

LoopClearBss:
    cmp r0, r1
    bcs RunMain
    str r2, [r0], #4
    b LoopClearBss

RunMain:
    /* Configure clocks/system state if overridden by the core. */
    bl SystemInit

    /* Publish the frequency SystemInit actually selected.
     *
     * SystemCoreClock is a .data variable initialised to
     * __SYSTEM_CLOCK_DEFAULT (64 MHz), and nothing in the MDK ever corrects
     * it, but SystemInit programs OSCILLATORS.PLL.FREQ to 128 MHz unless
     * NRF_CONFIG_CPU_FREQ_MHZ says otherwise. Everything that derives a time
     * from a cycle count -- micros(), delayMicroseconds() -- was therefore
     * working from a figure that is half the real one.
     *
     * SystemCoreClockUpdate() reads PLL.CURRENTFREQ, so it has to run after
     * SystemInit has set PLL.FREQ, and it has to run before
     * __libc_init_array so a global constructor that delays gets the right
     * answer too. .data is already copied at this point, so the value sticks.
     * The MDK's system_nrf54l.c provides the function; the weak stub at the
     * end of this file covers a build without it. */
    bl SystemCoreClockUpdate

    /* Run C/C++ runtime init so global constructors are executed. */
    bl __libc_init_array

    /* Enable interrupts */
    cpsie i

    /* Call main() */
    bl main

    /* main() should never return, but if it does, loop forever */
InfiniteLoop:
    b InfiniteLoop

    .size Reset_Handler, .-Reset_Handler

/* *****************************************************************************
 * Exception Handlers
 * *****************************************************************************/

    .weak NMI_Handler
    .type NMI_Handler, %function
NMI_Handler:
    b .
    .size NMI_Handler, .-NMI_Handler

    .weak HardFault_Handler
    .type HardFault_Handler, %function
HardFault_Handler:
    b .
    .size HardFault_Handler, .-HardFault_Handler

    .weak MemManage_Handler
    .type MemManage_Handler, %function
MemManage_Handler:
    b .
    .size MemManage_Handler, .-MemManage_Handler

    .weak BusFault_Handler
    .type BusFault_Handler, %function
BusFault_Handler:
    b .
    .size BusFault_Handler, .-BusFault_Handler

    .weak UsageFault_Handler
    .type UsageFault_Handler, %function
UsageFault_Handler:
    b .
    .size UsageFault_Handler, .-UsageFault_Handler

    .weak SecureFault_Handler
    .type SecureFault_Handler, %function
SecureFault_Handler:
    b .
    .size SecureFault_Handler, .-SecureFault_Handler

    .weak SVC_Handler
    .type SVC_Handler, %function
SVC_Handler:
    b .
    .size SVC_Handler, .-SVC_Handler

    .weak DebugMon_Handler
    .type DebugMon_Handler, %function
DebugMon_Handler:
    b .
    .size DebugMon_Handler, .-DebugMon_Handler

    .weak PendSV_Handler
    .type PendSV_Handler, %function
PendSV_Handler:
    b .
    .size PendSV_Handler, .-PendSV_Handler

    /* FreeRTOS uses the GRTC tickless port on nRF54L, not SysTick, so
     * SysTick_Handler is unused. Define a weak no-op so the vector slot
     * at offset 0x3C resolves; any code that does want a SysTick handler
     * (e.g. a bare-metal user sketch) can override this. */
    .weak SysTick_Handler
    .type SysTick_Handler, %function
SysTick_Handler:
    bx lr
    .size SysTick_Handler, .-SysTick_Handler

/* *****************************************************************************
 * Default Interrupt Handler
 * *****************************************************************************/

    .weak SWI00_IRQHandler
    .type SWI00_IRQHandler, %function
SWI00_IRQHandler:
    bx lr
    .size SWI00_IRQHandler, .-SWI00_IRQHandler

    .weak SWI01_IRQHandler
    .type SWI01_IRQHandler, %function
SWI01_IRQHandler:
    bx lr
    .size SWI01_IRQHandler, .-SWI01_IRQHandler

    .weak SWI02_IRQHandler
    .type SWI02_IRQHandler, %function
SWI02_IRQHandler:
    bx lr
    .size SWI02_IRQHandler, .-SWI02_IRQHandler

    .weak SWI03_IRQHandler
    .type SWI03_IRQHandler, %function
SWI03_IRQHandler:
    bx lr
    .size SWI03_IRQHandler, .-SWI03_IRQHandler

    .weak AAR00_CCM00_IRQHandler
    .type AAR00_CCM00_IRQHandler, %function
AAR00_CCM00_IRQHandler:
    bx lr
    .size AAR00_CCM00_IRQHandler, .-AAR00_CCM00_IRQHandler

    .weak ECB00_IRQHandler
    .type ECB00_IRQHandler, %function
ECB00_IRQHandler:
    bx lr
    .size ECB00_IRQHandler, .-ECB00_IRQHandler

    /* The rest of the SoftDevice-owned lines. nordic/sd_isr.S defines the real
     * forwarders; these weak defaults keep an undefined vector entry from
     * pulling some other archive member that happens to define the name --
     * nrfx_power.c defines CLOCK_POWER_IRQHandler, and loading it for that
     * drags in nrfx_clock.c and an unresolved nrfx_clock_lfclk_irq_handler. */

    .weak TIMER10_IRQHandler
    .type TIMER10_IRQHandler, %function
TIMER10_IRQHandler:
    bx lr
    .size TIMER10_IRQHandler, .-TIMER10_IRQHandler

    .weak RADIO_0_IRQHandler
    .type RADIO_0_IRQHandler, %function
RADIO_0_IRQHandler:
    bx lr
    .size RADIO_0_IRQHandler, .-RADIO_0_IRQHandler

    .weak GRTC_3_IRQHandler
    .type GRTC_3_IRQHandler, %function
GRTC_3_IRQHandler:
    bx lr
    .size GRTC_3_IRQHandler, .-GRTC_3_IRQHandler

    .weak CLOCK_POWER_IRQHandler
    .type CLOCK_POWER_IRQHandler, %function
CLOCK_POWER_IRQHandler:
    bx lr
    .size CLOCK_POWER_IRQHandler, .-CLOCK_POWER_IRQHandler

    .weak TWIM20_IRQHandler
    .type TWIM20_IRQHandler, %function
TWIM20_IRQHandler:
    b .
    .size TWIM20_IRQHandler, .-TWIM20_IRQHandler

    .weak TWIM21_IRQHandler
    .type TWIM21_IRQHandler, %function
TWIM21_IRQHandler:
    b .
    .size TWIM21_IRQHandler, .-TWIM21_IRQHandler

    .weak SERIAL00_IRQHandler
    .type SERIAL00_IRQHandler, %function
SERIAL00_IRQHandler:
    bx lr
    .size SERIAL00_IRQHandler, .-SERIAL00_IRQHandler

    .weak GRTC_0_IRQHandler
    .type GRTC_0_IRQHandler, %function
GRTC_0_IRQHandler:
    b .
    .size GRTC_0_IRQHandler, .-GRTC_0_IRQHandler

    .weak QDEC20_IRQHandler
    .type QDEC20_IRQHandler, %function
QDEC20_IRQHandler:
    bx lr
    .size QDEC20_IRQHandler, .-QDEC20_IRQHandler

    .weak QDEC21_IRQHandler
    .type QDEC21_IRQHandler, %function
QDEC21_IRQHandler:
    bx lr
    .size QDEC21_IRQHandler, .-QDEC21_IRQHandler

    /* Serial2 is only instantiated when a variant defines PIN_SERIAL2_*;
     * Wire only claims SERIAL22 in target mode. Both slots still have to
     * resolve when the variant leaves them unused. */
    .weak SERIAL20_IRQHandler
    .type SERIAL20_IRQHandler, %function
SERIAL20_IRQHandler:
    b .
    .size SERIAL20_IRQHandler, .-SERIAL20_IRQHandler

    .weak SERIAL21_IRQHandler
    .type SERIAL21_IRQHandler, %function
SERIAL21_IRQHandler:
    b .
    .size SERIAL21_IRQHandler, .-SERIAL21_IRQHandler

    .weak SERIAL22_IRQHandler
    .type SERIAL22_IRQHandler, %function
SERIAL22_IRQHandler:
    b .
    .size SERIAL22_IRQHandler, .-SERIAL22_IRQHandler

    .weak GRTC_1_IRQHandler
    .type GRTC_1_IRQHandler, %function
GRTC_1_IRQHandler:
    b .
    .size GRTC_1_IRQHandler, .-GRTC_1_IRQHandler

    .weak GRTC_2_IRQHandler
    .type GRTC_2_IRQHandler, %function
GRTC_2_IRQHandler:
    b .
    .size GRTC_2_IRQHandler, .-GRTC_2_IRQHandler

    /* GPIOTE20 has two IRQ lines, and which one attachInterrupt() uses depends
     * on GPIOTE_IRQ_GROUP (see WInterrupts.c): line 0 for a non-secure build,
     * line 1 for a secure one. WInterrupts.c defines whichever one is in use via
     * the GPIOTE_IRQHandler alias, so both vector slots need a weak default here
     * for the other one to resolve. */
    .weak GPIOTE20_0_IRQHandler
    .type GPIOTE20_0_IRQHandler, %function
GPIOTE20_0_IRQHandler:
    b .
    .size GPIOTE20_0_IRQHandler, .-GPIOTE20_0_IRQHandler

    .weak GPIOTE20_1_IRQHandler
    .type GPIOTE20_1_IRQHandler, %function
GPIOTE20_1_IRQHandler:
    b .
    .size GPIOTE20_1_IRQHandler, .-GPIOTE20_1_IRQHandler

    .weak Default_Handler
    .type Default_Handler, %function
Default_Handler:
    b .
    .size Default_Handler, .-Default_Handler

/* *****************************************************************************
 * System Init (optional - can be overridden)
 * *****************************************************************************/

    .weak SystemInit
    .type SystemInit, %function
SystemInit:
    bx lr
    .size SystemInit, .-SystemInit

    .weak SystemCoreClockUpdate
    .type SystemCoreClockUpdate, %function
SystemCoreClockUpdate:
    bx lr
    .size SystemCoreClockUpdate, .-SystemCoreClockUpdate

#endif
