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

#include "py/obj.h"

#ifndef __MICROPY_INCLUDED_S1MOD_MODMACHINE_H__
#define __MICROPY_INCLUDED_S1MOD_MODMACHINE_H__

/**
 * @brief Declaration of the ADC class.
 */
extern const mp_obj_type_t machine_adc_type;

/**
 * @brief Declaration of the Flash class.
 */
extern const mp_obj_type_t machine_flash_type;

/**
 * @brief Declaration of the FPGA class.
 */
extern const mp_obj_type_t machine_fpga_type;

/**
 * @brief Declaration of the PMIC class.
 */
extern const mp_obj_type_t machine_pmic_type;

/**
 * @brief Declaration of the Pin class.
 */
extern const mp_obj_type_t machine_pin_type;

/**
 * @brief Declaration of the RTC class.
 */
extern const mp_obj_type_t machine_rtc_type;

/**
 * @brief Initialises the FPGA module.
 */
void machine_fpga_init(void);

/**
 * @brief Initialises the PMIC module.
 */
void machine_pmic_init(void);

/**
 * @brief Initialises the RTC module.
 */
void machine_rtc_init(void);

#endif