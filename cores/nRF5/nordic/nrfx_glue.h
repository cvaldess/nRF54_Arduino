/*
 * nrfx 4.x glue layer for nRF54L Arduino / FreeRTOS environment
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Bridges between the nrfx hardware abstraction layer and the
 * Arduino core running on Cortex-M33 (nRF54L series).
 */

#ifndef NRFX_GLUE_H__
#define NRFX_GLUE_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup nrfx_glue nrfx_glue.h
 * @{
 * @ingroup nrfx
 *
 * @brief This file contains macros that should be implemented according to
 *        the needs of the host environment into which @em nrfx is integrated.
 */

// ---------------------------------------------------------------------------
// IRQ handler binding (link-time via MDK vector table)
// ---------------------------------------------------------------------------
#include <bsp/stable/soc/nrfx_irqs.h>

// ---------------------------------------------------------------------------
// Assertions
// ---------------------------------------------------------------------------

/** @brief Macro for placing a runtime assertion. */
#define NRFX_ASSERT(expression)

/** @brief Macro for placing a compile-time assertion. */
#define NRFX_STATIC_ASSERT(expression) _Static_assert(expression, "NRFX_STATIC_ASSERT")

// ---------------------------------------------------------------------------
// IRQ management  (NVIC, Cortex-M33)
// ---------------------------------------------------------------------------

/**
 * @brief Macro for setting the priority of a specific IRQ.
 *
 * @param irq_number  IRQ number.
 * @param priority    Priority to set.
 */
#define NRFX_IRQ_PRIORITY_SET(irq_number, priority) \
    _NRFX_IRQ_PRIORITY_SET(irq_number, priority)
static inline void _NRFX_IRQ_PRIORITY_SET(IRQn_Type irq_number,
                                           uint8_t   priority)
{
    NVIC_SetPriority(irq_number, priority);
}

/**
 * @brief Macro for enabling a specific IRQ.
 *
 * @param irq_number  IRQ number.
 */
#define NRFX_IRQ_ENABLE(irq_number) _NRFX_IRQ_ENABLE(irq_number)
static inline void _NRFX_IRQ_ENABLE(IRQn_Type irq_number)
{
    NVIC_ClearPendingIRQ(irq_number);
    NVIC_EnableIRQ(irq_number);
}

/**
 * @brief Macro for checking if a specific IRQ is enabled.
 *
 * @param irq_number  IRQ number.
 *
 * @retval true  If the IRQ is enabled.
 * @retval false Otherwise.
 */
#define NRFX_IRQ_IS_ENABLED(irq_number) _NRFX_IRQ_IS_ENABLED(irq_number)
static inline bool _NRFX_IRQ_IS_ENABLED(IRQn_Type irq_number)
{
    return 0 != (NVIC->ISER[irq_number / 32] & (1UL << (irq_number % 32)));
}

/**
 * @brief Macro for disabling a specific IRQ.
 *
 * @param irq_number  IRQ number.
 */
#define NRFX_IRQ_DISABLE(irq_number) _NRFX_IRQ_DISABLE(irq_number)
static inline void _NRFX_IRQ_DISABLE(IRQn_Type irq_number)
{
    NVIC_DisableIRQ(irq_number);
}

/**
 * @brief Macro for setting a specific IRQ as pending.
 *
 * @param irq_number  IRQ number.
 */
#define NRFX_IRQ_PENDING_SET(irq_number) _NRFX_IRQ_PENDING_SET(irq_number)
static inline void _NRFX_IRQ_PENDING_SET(IRQn_Type irq_number)
{
    NVIC_SetPendingIRQ(irq_number);
}

/**
 * @brief Macro for clearing the pending status of a specific IRQ.
 *
 * @param irq_number  IRQ number.
 */
#define NRFX_IRQ_PENDING_CLEAR(irq_number) _NRFX_IRQ_PENDING_CLEAR(irq_number)
static inline void _NRFX_IRQ_PENDING_CLEAR(IRQn_Type irq_number)
{
    NVIC_ClearPendingIRQ(irq_number);
}

/**
 * @brief Macro for checking the pending status of a specific IRQ.
 *
 * @param irq_number  IRQ number.
 *
 * @retval true  If the IRQ is pending.
 * @retval false Otherwise.
 */
#define NRFX_IRQ_IS_PENDING(irq_number) _NRFX_IRQ_IS_PENDING(irq_number)
static inline bool _NRFX_IRQ_IS_PENDING(IRQn_Type irq_number)
{
    return (NVIC_GetPendingIRQ(irq_number) == 1);
}

// ---------------------------------------------------------------------------
// Critical sections  (global interrupt disable/enable)
// ---------------------------------------------------------------------------

#define NRFX_CRITICAL_SECTION_ENTER()   \
    do {                                 \
        unsigned int _nrfx_primask = __get_PRIMASK(); \
        __disable_irq();

#define NRFX_CRITICAL_SECTION_EXIT()    \
        __set_PRIMASK(_nrfx_primask);   \
    } while (0)

// ---------------------------------------------------------------------------
// Delay
// ---------------------------------------------------------------------------

/*
 * nrfx_coredep_delay_us() spins on a SUBS/BHI pair and converts microseconds to
 * iterations assuming NRFX_COREDEP_DELAY_US_LOOP_CYCLES (3) cycles per
 * iteration. Executed from nRF54L's RRAM that loop costs closer to eight
 * cycles, so the delay it produces is more than twice what was asked for.
 *
 * nrf54_delay_us() (cores/nRF5/delay.c) counts real cycles with the DWT cycle
 * counter instead, which cannot be wrong about how long an instruction took,
 * and falls back to the loop when the cycle counter is unavailable. Route the
 * whole of nrfx through it, so a driver's NRFX_DELAY_US() and the sketch's
 * delayMicroseconds() agree.
 *
 * NRFX_COREDEP_DELAY_DWT_BASED is deliberately left off: it would give nrfx
 * the same DWT loop but with no fallback, and ARMv8-M makes
 * DWT_CTRL.CYCCNTENA RAZ/WI when non-invasive debug is not permitted -- the
 * counter then never advances and the wait never ends.
 *
 * NRFX_COREDEP_DELAY_US_LOOP_CYCLES is likewise left at the nrfx default of 3
 * rather than corrected to the eight the loop really costs, which leaves the
 * fallback running ~2.7x long. That is worth it: the fallback is only reached
 * where the cycle counter is dead, long is the safe direction for a delay, and
 * the eight is an instruction-fetch cost measured on an nRF54LM20A. The loop
 * already scales itself by SystemCoreClock, so the constant is not about clock
 * rate -- it is about how fast the part feeds the core from RRAM, and nobody
 * has measured that on the nRF54L15 this core also builds for.
 *
 * Routing NRFX_DELAY_US() does not catch every nrfx delay. nrfy_grtc_prepare()
 * calls nrfx_coredep_delay_us(93) directly -- upstream avoids the macro there
 * because under Zephyr it would need a system timer that has not started yet
 * -- and nrfx_cracen.c does the same twice for 1 us. None of the three are
 * built in this configuration: the cracen pair sit inside
 * NRFX_CRACEN_BSIM_SUPPORT, which is defined nowhere, and the GRTC call is
 * reachable only from nrfx_grtc.c, whose every line is inside
 * #if NRFX_CHECK(NRFX_GRTC_ENABLED) -- and nrfx_config.h sets that to 0. The
 * uncorrected constant costs nothing as configured; it would start to matter
 * if either driver were ever enabled.
 */
#include <lib/nrfx_coredep.h>

#include <stdint.h>
void nrf54_delay_us(uint32_t us);

/**
 * @brief Macro for delaying the code execution for at least the specified time.
 *
 * @param us_time Number of microseconds to wait.
 */
#define NRFX_DELAY_US(us_time)  nrf54_delay_us(us_time)

// ---------------------------------------------------------------------------
// Error codes
// ---------------------------------------------------------------------------

/**
 * @brief When set to a non-zero value, this macro specifies that the
 *        @ref nrfx_error_codes and the @ref nrfx_err_t type itself are defined
 *        in a customized way and the default definitions from <nrfx_error.h>
 *        should not be used.
 */
#define NRFX_CUSTOM_ERROR_CODES 0

// ---------------------------------------------------------------------------
// Atomic operations  (Cortex-M33 exclusive access)
// ---------------------------------------------------------------------------

/** @brief Atomic 32-bit unsigned type. */
typedef volatile uint32_t nrfx_atomic_t;

/**
 * @brief Macro for storing a value to an atomic object and returning its previous value.
 *
 * @param[in] p_data  Pointer to the atomic object.
 * @param[in] value   Value to store.
 *
 * @return Previous value stored in the atomic object.
 */
#define NRFX_ATOMIC_FETCH_STORE(p_data, value) \
    nrfx_atomic_fetch_store(p_data, value)

static inline uint32_t nrfx_atomic_fetch_store(nrfx_atomic_t * p_data,
                                                uint32_t        value)
{
    uint32_t prev;
    uint32_t status;
    do {
        prev   = __LDREXW(p_data);
        status = __STREXW(value, p_data);
    } while (status != 0);
    __DMB();
    return prev;
}

/**
 * @brief Macro for performing a logical OR on an atomic object and returning
 *        its previous value.
 *
 * @param[in] p_data  Pointer to the atomic object.
 * @param[in] value   Value to OR.
 *
 * @return Previous value stored in the atomic object.
 */
#define NRFX_ATOMIC_FETCH_OR(p_data, value) \
    nrfx_atomic_fetch_or(p_data, value)

static inline uint32_t nrfx_atomic_fetch_or(nrfx_atomic_t * p_data,
                                             uint32_t        value)
{
    uint32_t prev;
    uint32_t status;
    do {
        prev   = __LDREXW(p_data);
        status = __STREXW(prev | value, p_data);
    } while (status != 0);
    __DMB();
    return prev;
}

/**
 * @brief Macro for performing a logical AND on an atomic object and returning
 *        its previous value.
 *
 * @param[in] p_data  Pointer to the atomic object.
 * @param[in] value   Value to AND.
 *
 * @return Previous value stored in the atomic object.
 */
#define NRFX_ATOMIC_FETCH_AND(p_data, value) \
    nrfx_atomic_fetch_and(p_data, value)

static inline uint32_t nrfx_atomic_fetch_and(nrfx_atomic_t * p_data,
                                              uint32_t        value)
{
    uint32_t prev;
    uint32_t status;
    do {
        prev   = __LDREXW(p_data);
        status = __STREXW(prev & value, p_data);
    } while (status != 0);
    __DMB();
    return prev;
}

/**
 * @brief Macro for performing a logical XOR on an atomic object and returning
 *        its previous value.
 *
 * @param[in] p_data  Pointer to the atomic object.
 * @param[in] value   Value to XOR.
 *
 * @return Previous value stored in the atomic object.
 */
#define NRFX_ATOMIC_FETCH_XOR(p_data, value) \
    nrfx_atomic_fetch_xor(p_data, value)

static inline uint32_t nrfx_atomic_fetch_xor(nrfx_atomic_t * p_data,
                                              uint32_t        value)
{
    uint32_t prev;
    uint32_t status;
    do {
        prev   = __LDREXW(p_data);
        status = __STREXW(prev ^ value, p_data);
    } while (status != 0);
    __DMB();
    return prev;
}

/**
 * @brief Macro for performing an addition on an atomic object and returning
 *        its previous value.
 *
 * @param[in] p_data  Pointer to the atomic object.
 * @param[in] value   Value to add.
 *
 * @return Previous value stored in the atomic object.
 */
#define NRFX_ATOMIC_FETCH_ADD(p_data, value) \
    nrfx_atomic_fetch_add(p_data, value)

static inline uint32_t nrfx_atomic_fetch_add(nrfx_atomic_t * p_data,
                                              uint32_t        value)
{
    uint32_t prev;
    uint32_t status;
    do {
        prev   = __LDREXW(p_data);
        status = __STREXW(prev + value, p_data);
    } while (status != 0);
    __DMB();
    return prev;
}

/**
 * @brief Macro for performing a subtraction on an atomic object and returning
 *        its previous value.
 *
 * @param[in] p_data  Pointer to the atomic object.
 * @param[in] value   Value to subtract.
 *
 * @return Previous value stored in the atomic object.
 */
#define NRFX_ATOMIC_FETCH_SUB(p_data, value) \
    nrfx_atomic_fetch_sub(p_data, value)

static inline uint32_t nrfx_atomic_fetch_sub(nrfx_atomic_t * p_data,
                                              uint32_t        value)
{
    uint32_t prev;
    uint32_t status;
    do {
        prev   = __LDREXW(p_data);
        status = __STREXW(prev - value, p_data);
    } while (status != 0);
    __DMB();
    return prev;
}

/**
 * @brief Macro for performing a compare-and-swap on an atomic object.
 *
 * @param[in] p_data    Pointer to the atomic object.
 * @param[in] old_value Expected old value.
 * @param[in] new_value New value to store if *p_data == old_value.
 *
 * @retval true  If the swap was performed (old value matched).
 * @retval false Otherwise.
 */
#define NRFX_ATOMIC_CAS(p_data, old_value, new_value) \
    nrfx_atomic_cas(p_data, old_value, new_value)

static inline bool nrfx_atomic_cas(nrfx_atomic_t * p_data,
                                    uint32_t        old_value,
                                    uint32_t        new_value)
{
    uint32_t prev;
    uint32_t status;
    prev = __LDREXW(p_data);
    if (prev != old_value)
    {
        __CLREX();
        return false;
    }
    status = __STREXW(new_value, p_data);
    __DMB();
    return (status == 0);
}

// ---------------------------------------------------------------------------
// DPPI channels / groups used (replaces PPI on nRF54L)
// ---------------------------------------------------------------------------

/** @brief Bitmask defining DPPI channels reserved to be used outside of nrfx. */
#define NRFX_DPPI_CHANNELS_USED  0

/** @brief Bitmask defining DPPI groups reserved to be used outside of nrfx. */
#define NRFX_DPPI_GROUPS_USED    0

// ---------------------------------------------------------------------------
// Legacy PPI macros (kept for compatibility with shared nrfx helpers)
// ---------------------------------------------------------------------------

/** @brief Bitmask defining PPI channels reserved to be used outside of nrfx. */
#define NRFX_PPI_CHANNELS_USED   0

/** @brief Bitmask defining PPI groups reserved to be used outside of nrfx. */
#define NRFX_PPI_GROUPS_USED     0

// ---------------------------------------------------------------------------
// EGU / SWI / TIMER instances reserved outside of nrfx
// ---------------------------------------------------------------------------

/** @brief Bitmask defining EGU instances reserved to be used outside of nrfx. */
#define NRFX_EGUS_USED           0

/** @brief Bitmask defining SWI instances reserved to be used outside of nrfx. */
#define NRFX_SWI_USED            0

/** @brief Bitmask defining TIMER instances reserved to be used outside of nrfx. */
#define NRFX_TIMERS_USED         0

/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRFX_GLUE_H__
