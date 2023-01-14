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

#include "py/runtime.h"
#include "nrfx_rtc.h"
#include "nrf_soc.h"

/**
 * @brief Instance of the RTC1 driver. Note that RTC0 is used by the softdevice.
 */
static const nrfx_rtc_t rtc_instance = NRFX_RTC_INSTANCE(1);

/**
 * @brief Epoch reference time in seconds. Updated every hour internally, or
 *        when RTC.time() is called with an argument.
 */
static uint32_t epoch_time_ref;

/**
 * @brief Flag which is set during RTC.sleep_ms()
 */
static bool waiting;

/**
 * @brief Forward declaration of the RTC class object.
 */
const mp_obj_type_t machine_rtc_type;

/**
 * @brief RTC IRQ handler
 */
void rtc_irq_handler(nrfx_rtc_int_type_t int_type)
{
    switch (int_type)
    {
    // Used to increment the reference time every hour
    case NRFX_RTC_INT_COMPARE0:

        // Increment the reference time by 1 hour
        epoch_time_ref += 60 * 60;

        // Clear the counter
        nrfx_rtc_counter_clear(&rtc_instance);

        // Restart the interrupt
        nrfx_rtc_int_enable(&rtc_instance, NRF_RTC_INT_COMPARE0_MASK);

        break;

    // User configurable for delay_ms function
    case NRFX_RTC_INT_COMPARE1:

        // Clear the waiting flag
        waiting = false;

        // Disable the timer
        nrfx_rtc_cc_disable(&rtc_instance, 1);

        break;

    default:
        break;
    }
}

/**
 * @brief Initialises the RTC module.
 */
void machine_rtc_init(void)
{
    // Configure the RTC1 timer to a 1ms tick
    nrfx_rtc_config_t config = {
        .prescaler = 32,
        .interrupt_priority = NRFX_RTC_DEFAULT_CONFIG_IRQ_PRIORITY,
        .tick_latency = 32,
        .reliable = false,
    };
    nrfx_rtc_init(&rtc_instance, &config, rtc_irq_handler);

    // Set the compare 0 to interrupt every hour. Value here is in ms
    nrfx_rtc_cc_set(&rtc_instance, 0, 3600000, true);

    // Enable the RTC
    nrfx_rtc_enable(&rtc_instance);
}

/**
 * @brief Returns a the current time since power on in seconds. If an argument
 *        is provided. The current time will be updated to that value. Not this
 *        won't work with the Unix Epoch time, as the micropython int size is
 *        only 2^30. A different reference should be used. Such as seconds since
 *        1st Jan 2000.
 */
STATIC mp_obj_t machine_rtc_time(size_t n_args, const mp_obj_t *args)
{
    // If no arguments are given, return the time as a tuple
    if (n_args == 0)
    {
        // Get the current counter and add the reference time
        uint32_t time = (nrfx_rtc_counter_get(&rtc_instance) / 1000) + // (ms)
                        epoch_time_ref;                                // (sec)

        // Return the values
        return MP_OBJ_NEW_SMALL_INT(time);
    }

    // Otherwise, if a value was provided, set the time
    epoch_time_ref = mp_obj_get_int(args[0]);

    // Clear the current counter
    nrfx_rtc_counter_clear(&rtc_instance);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_rtc_time_obj, 0, 1, machine_rtc_time);

/**
 * @brief Keeps the device asleep for a number of ms provided.
 */
STATIC mp_obj_t machine_rtc_sleep_ms(mp_obj_t self_in)
{
    // Get the current counter value
    uint32_t counter_val = nrfx_rtc_counter_get(&rtc_instance);

    // Set the wake time to be the current counter value + the time requested
    uint32_t wake_time = counter_val + mp_obj_get_int(self_in);

    // If the wake time will be later than 1 hour, we need to deduct 1 hour to
    // compensate for the 1 hour periodic rollover. Calculated in ms
    if (wake_time > 3600000)
    {
        wake_time -= 3600000;
    }

    // Set the compare 1 interrupt to trigger after the given time
    nrfx_rtc_cc_set(&rtc_instance, 1, wake_time, true);

    // Set the waiting flag to true
    waiting = true;

    // While waiting, stay asleep
    while (waiting)
    {
        // Set to low power mode
        sd_power_mode_set(NRF_POWER_MODE_LOWPWR);

        // Wait for events to save power
        // sd_app_evt_wait(); TODO figure out why this doesn't work
        __WFI();
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_rtc_sleep_ms_obj, machine_rtc_sleep_ms);

/**
 * @brief Local class dictionary. Contains all the methods and constants of RTC.
 */
STATIC const mp_rom_map_elem_t machine_rtc_locals_dict_table[] = {

    // Class methods
    {MP_ROM_QSTR(MP_QSTR_time), MP_ROM_PTR(&machine_rtc_time_obj)},
    {MP_ROM_QSTR(MP_QSTR_sleep_ms), MP_ROM_PTR(&machine_rtc_sleep_ms_obj)},
};
STATIC MP_DEFINE_CONST_DICT(machine_rtc_locals_dict, machine_rtc_locals_dict_table);

/**
 * @brief Class structure for the RTC object.
 */
const mp_obj_type_t machine_rtc_type = {
    .base = {&mp_type_type},
    .name = MP_QSTR_RTC,
    .print = NULL,
    .make_new = NULL,
    .call = NULL,
    .locals_dict = (mp_obj_dict_t *)&machine_rtc_locals_dict,
};
