/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2024 Adafruit Industries
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

#include "nRF54Crypto.h"

#ifdef NRF_CRACEN

#include "nrf_cracen.h"
#include "nrf_cracen_rng.h"

#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>

#include <tinycrypt/ecc.h>
#include <tinycrypt/ecc_dh.h>
#include <tinycrypt/ecc_platform_specific.h>
#include <tinycrypt/constants.h>

// Timeout for RNG FIFO fill (in loop iterations)
#define RNG_TIMEOUT 100000

nRF54CryptoClass nRF54Crypto;

//--------------------------------------------------------------------+
// CRACEN RNG
//--------------------------------------------------------------------+

// Enable with 1x 128-bit block for AES conditioning
static void cracen_rng_control(bool soft_reset)
{
    nrf_cracen_rng_control_t cfg = { 0 };
    cfg.enable = true;
    cfg.number_128_blocks = 1;
    cfg.soft_reset = soft_reset;
    nrf_cracen_rng_control_set(NRF_CRACENCORE, &cfg);
}

// A halted generator needs a soft reset before it produces again
static void cracen_rng_reset_if_halted(void)
{
    if (nrf_cracen_rng_fsm_state_get(NRF_CRACENCORE) != NRF_CRACEN_RNG_FSM_STATE_ERROR) return;
    cracen_rng_control(true);
    cracen_rng_control(false);
}

// CRACEN->ENABLE also holds the CRYPTOMASTER and PKE/IKG bits, which an application may be using
// through nrfx; the read-modify-write must not interleave with one of its own.
static void cracen_rng_module_set(bool on)
{
    taskENTER_CRITICAL();
    if (on) {
        nrf_cracen_module_enable(NRF_CRACEN, NRF_CRACEN_MODULE_RNG_MASK);
    } else {
        nrf_cracen_module_disable(NRF_CRACEN, NRF_CRACEN_MODULE_RNG_MASK);
    }
    taskEXIT_CRITICAL();
}

// Turns the generator and its module off, whether it got as far as starting or not
static void cracen_rng_disable(void)
{
    nrf_cracen_rng_control_t cfg = { 0 };
    cfg.enable = false;
    nrf_cracen_rng_control_set(NRF_CRACENCORE, &cfg);

    cracen_rng_module_set(false);
}

static bool cracen_rng_start(void)
{
    cracen_rng_module_set(true);
    cracen_rng_control(false);

    // Wait for the FSM to leave RESET/STARTUP; a full FIFO parks it in IDLE_STANDBY (rings off)
    uint32_t timeout = RNG_TIMEOUT;
    while (timeout--) {
        nrf_cracen_rng_fsm_state_t state = nrf_cracen_rng_fsm_state_get(NRF_CRACENCORE);
        if (state == NRF_CRACEN_RNG_FSM_STATE_IDLE_READY ||
            state == NRF_CRACEN_RNG_FSM_STATE_IDLE_STANDBY ||
            state == NRF_CRACEN_RNG_FSM_STATE_FILL_FIFO) {
            return true;
        }
        cracen_rng_reset_if_halted();
    }

    // The module is already on: a start that times out must not leave it running
    cracen_rng_disable();
    return false;
}

// One draw at a time, and the TRNG runs only for the draw. The mutex is taken only while the
// scheduler runs: before it starts, or while it is suspended, no other task can get in, and blocking
// there would trip FreeRTOS's assert.
static StaticSemaphore_t _rng_lock_buf;
static SemaphoreHandle_t _rng_lock = xSemaphoreCreateMutexStatic(&_rng_lock_buf);

static bool rng_lock(void)
{
    if (xTaskGetSchedulerState() != taskSCHEDULER_RUNNING) return false;
    (void) xSemaphoreTake(_rng_lock, portMAX_DELAY);
    return true;
}

static void rng_unlock(bool locked)
{
    if (locked) (void) xSemaphoreGive(_rng_lock);
}

static bool cracen_rng_word(uint32_t *word)
{
    if (nrf_cracen_rng_fifo_level_get(NRF_CRACENCORE) == 0) {
        cracen_rng_reset_if_halted();
        return false;
    }
    *word = nrf_cracen_rng_fifo_get(NRF_CRACENCORE);
    return true;
}

static bool cracen_rng_fill(uint8_t *dest, size_t len)
{
    size_t offset = 0;
    while (offset < len) {
        uint32_t word;
        uint32_t timeout = RNG_TIMEOUT;
        while (!cracen_rng_word(&word)) {
            if (--timeout == 0) return false;
        }

        // Copy bytes (handle partial word at end)
        size_t remaining = len - offset;
        size_t to_copy = (remaining < 4) ? remaining : 4;
        memcpy(dest + offset, &word, to_copy);
        offset += to_copy;
    }
    return true;
}

//--------------------------------------------------------------------+
// default_CSPRNG - called by TinyCrypt uECC
//--------------------------------------------------------------------+

static bool rng_draw(uint8_t *dest, size_t len)
{
    bool locked = rng_lock();
    // A failed start has already turned the TRNG off
    bool ok = cracen_rng_start();
    if (ok) {
        ok = cracen_rng_fill(dest, len);
        cracen_rng_disable();
    }
    rng_unlock(locked);
    return ok;
}

extern "C" int default_CSPRNG(uint8_t *dest, unsigned int size)
{
    return rng_draw(dest, size) ? 1 : 0;
}

//--------------------------------------------------------------------+
// nRF54CryptoClass
//--------------------------------------------------------------------+

// The TRNG starts and stops around each draw, and uECC already defaults to default_CSPRNG
// (ecc_platform_specific.h), so begin() and end() have nothing to do. Both stay for the callers that
// pair them.
bool nRF54CryptoClass::begin(void)
{
    return true;
}

void nRF54CryptoClass::end(void)
{
}

bool nRF54CryptoClass::genKeyPair(uint8_t privkey[32], uint8_t pubkey[64])
{
    return uECC_make_key(pubkey, privkey, uECC_secp256r1()) == TC_CRYPTO_SUCCESS;
}

bool nRF54CryptoClass::sharedSecret(const uint8_t peer_pubkey[64],
                                     const uint8_t privkey[32],
                                     uint8_t dhkey[32])
{
    return uECC_shared_secret(peer_pubkey, privkey, dhkey, uECC_secp256r1()) == TC_CRYPTO_SUCCESS;
}

bool nRF54CryptoClass::random(uint8_t* dest, size_t len)
{
    return rng_draw(dest, len);
}

#endif // NRF_CRACEN
