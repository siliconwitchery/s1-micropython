/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 Raj Nakarja - Silicon Witchery AB
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

#ifndef NRFX_GLUE_H__
#define NRFX_GLUE_H__

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Macro for placing a runtime assertion.
 * @param expression Expression to be evaluated.
 */
#define NRFX_ASSERT(expression) \
    do                          \
    {                           \
        bool res = expression;  \
        (void)res;              \
    } while (0)

/**
 * @brief Macro for placing a compile time assertion.
 * @param expression Expression to be evaluated.
 */
#define NRFX_STATIC_ASSERT(expression) \
    _Static_assert(expression, "unspecified message")

    //------------------------------------------------------------------------------

#include "nrf_nvic.h"

/**
 * @brief Macro for setting the priority of a specific IRQ.
 * @param irq_number IRQ number.
 * @param priority   Priority to be set.
 */
#define NRFX_IRQ_PRIORITY_SET(irq_number, priority) sd_nvic_SetPriority(irq_number, priority)

/**
 * @brief Macro for enabling a specific IRQ.
 * @param irq_number IRQ number.
 */
#define NRFX_IRQ_ENABLE(irq_number) sd_nvic_EnableIRQ(irq_number)

/**
 * @brief Macro for checking if a specific IRQ is enabled.
 * @param irq_number IRQ number.
 * @retval true  If the IRQ is enabled.
 * @retval false Otherwise.
 */
#define NRFX_IRQ_IS_ENABLED(irq_number) (0 != (NVIC->ISER[irq_number / 32] & (1UL << (irq_number % 32))))

/**
 * @brief Macro for disabling a specific IRQ.
 * @param irq_number IRQ number.
 */
#define NRFX_IRQ_DISABLE(irq_number) sd_nvic_DisableIRQ(irq_number)

/**
 * @brief Macro for setting a specific IRQ as pending.
 * @param irq_number IRQ number.
 */
#define NRFX_IRQ_PENDING_SET(irq_number) sd_nvic_SetPendingIRQ(irq_number)

/**
 * @brief Macro for clearing the pending status of a specific IRQ.
 * @param irq_number IRQ number.
 */
#define NRFX_IRQ_PENDING_CLEAR(irq_number) sd_nvic_ClearPendingIRQ(irq_number)

/**
 * @brief Macro for checking the pending status of a specific IRQ.
 * @retval true  If the IRQ is pending.
 * @retval false Otherwise.
 */
#define NRFX_IRQ_IS_PENDING(irq_number) NVIC_GetPendingIRQ(irq_number)

/**
 * @brief Macro for entering into a critical section.
 * */
#define NRFX_CRITICAL_SECTION_ENTER()       \
    {                                       \
        uint8_t _is_nested_critical_region; \
        sd_nvic_critical_region_enter(&_is_nested_critical_region);

/**
 * @brief Macro for exiting from a critical section.
 * */
#define NRFX_CRITICAL_SECTION_EXIT()                          \
    sd_nvic_critical_region_exit(_is_nested_critical_region); \
    }

    //------------------------------------------------------------------------------

/**
 * @brief Macro for delaying the code execution for at least the specified time.
 * @param us_time Number of microseconds to wait.
 */
#include "nrfx.h"
#include <soc/nrfx_coredep.h>
#define NRFX_DELAY_US(us_time) nrfx_coredep_delay_us(us_time)

    //------------------------------------------------------------------------------

#include <soc/nrfx_atomic.h>

/**
 * @brief Atomic 32-bit unsigned type.
 */
#define nrfx_atomic_t nrfx_atomic_u32_t

/**
 * @brief Macro for storing a value to an atomic object and returning its previous value.
 * @param[in] p_data Atomic memory pointer.
 * @param[in] value  Value to store.
 * @return Previous value of the atomic object.
 */
#define NRFX_ATOMIC_FETCH_STORE(p_data, value) nrfx_atomic_u32_fetch_store(p_data, value)

/**
 * @brief Macro for running a bitwise OR operation on an atomic object and returning its previous value.
 * @param[in] p_data Atomic memory pointer.
 * @param[in] value  Value of the second operand in the OR operation.
 * @return Previous value of the atomic object.
 */
#define NRFX_ATOMIC_FETCH_OR(p_data, value) nrfx_atomic_u32_fetch_or(p_data, value)

/**
 * @brief Macro for running a bitwise AND operation on an atomic object
 *        and returning its previous value.
 * @param[in] p_data Atomic memory pointer.
 * @param[in] value  Value of the second operand in the AND operation.
 * @return Previous value of the atomic object.
 */
#define NRFX_ATOMIC_FETCH_AND(p_data, value) nrfx_atomic_u32_fetch_and(p_data, value)

/**
 * @brief Macro for running a bitwise XOR operation on an atomic object
 *        and returning its previous value.
 * @param[in] p_data Atomic memory pointer.
 * @param[in] value  Value of the second operand in the XOR operation.
 * @return Previous value of the atomic object.
 */
#define NRFX_ATOMIC_FETCH_XOR(p_data, value) nrfx_atomic_u32_fetch_xor(p_data, value)

/**
 * @brief Macro for running an addition operation on an atomic object
 *        and returning its previous value.
 * @param[in] p_data Atomic memory pointer.
 * @param[in] value  Value of the second operand in the ADD operation.
 * @return Previous value of the atomic object.
 */
#define NRFX_ATOMIC_FETCH_ADD(p_data, value) nrfx_atomic_u32_fetch_add(p_data, value)

/**
 * @brief Macro for running a subtraction operation on an atomic object
 *        and returning its previous value.
 * @param[in] p_data Atomic memory pointer.
 * @param[in] value  Value of the second operand in the SUB operation.
 * @return Previous value of the atomic object.
 */
#define NRFX_ATOMIC_FETCH_SUB(p_data, value) nrfx_atomic_u32_fetch_sub(p_data, value)

//------------------------------------------------------------------------------

/**
 * @brief When set to a non-zero value, this macro specifies that the
 *        @ref nrfx_error_codes and the @ref nrfx_err_t type itself are defined
 *        in a customized way and the default definitions from @c <nrfx_error.h>
 *        should not be used.
 */
#define NRFX_CUSTOM_ERROR_CODES 0

//------------------------------------------------------------------------------

/**
 * @brief When set to a non-zero value, this macro specifies that inside HALs
 *        the event registers are read back after clearing, on devices that
 *        otherwise could defer the actual register modification.
 */
#define NRFX_EVENT_READBACK_ENABLED 1

//------------------------------------------------------------------------------

/**
 *  @brief Bitmask that defines DPPI channels that are reserved for use outside of the nrfx library.
 */
#define NRFX_DPPI_CHANNELS_USED 0

/**
 * @brief Bitmask that defines DPPI groups that are reserved for use outside of the nrfx library.
 */
#define NRFX_DPPI_GROUPS_USED 0

/**
 * @brief Bitmask that defines PPI channels that are reserved for use outside of the nrfx library.
 */
#define NRFX_PPI_CHANNELS_USED 0

/**
 * @brief Bitmask that defines PPI groups that are reserved for use outside of the nrfx library.
 */
#define NRFX_PPI_GROUPS_USED 0

/**
 * @brief Bitmask that defines GPIOTE channels that are reserved for use outside of the nrfx library.
 */
#define NRFX_GPIOTE_CHANNELS_USED 0

/**
 * @brief Bitmask that defines EGU instances that are reserved for use outside of the nrfx library.
 */
#define NRFX_EGUS_USED 0

/**
 * @brief Bitmask that defines TIMER instances that are reserved for use outside of the nrfx library.
 */
#define NRFX_TIMERS_USED 0

/**
 * @brief Connect the standard GPIOTE_IRQ handler to the nrfx one.
 */
#define nrfx_gpiote_irq_handler GPIOTE_IRQHandler

/**
 * @brief Connect the standard RTC1_IRQ handler to the nrfx one.
 */
#define nrfx_rtc_1_irq_handler RTC1_IRQHandler

#ifdef __cplusplus
}
#endif

#endif // NRFX_GLUE_H__