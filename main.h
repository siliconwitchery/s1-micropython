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

#ifndef __MICROPY_INCLUDED_S1MOD_MAIN_H__
#define __MICROPY_INCLUDED_S1MOD_MAIN_H__

#include <stdint.h>

// TODO do we want a list of error codes?

/**
 * @brief Assert function.
 * @param err: If err is not 0, the error will be logged, and chip will reset.
 */
void assert_if(uint32_t err);

/**
 * @brief Enum for selecting which device to communicate with using the
 *        spim_tx_rx() function.
 */
typedef enum
{
    FPGA,
    FLASH,
} spi_device_t;

/**
 * @brief Function for communicating with the Flash and FPGA.
 * @param tx_buffer: Pointer to the transmit data buffer
 * @param tx_len: How many bytes to transmit
 * @param rx_buffer: Pointer to the received data buffer
 * @param rx_len: How many bytes to receive
 * @param device: Which device to communicate with
 */
void spim_tx_rx(uint8_t *tx_buffer, size_t tx_len,
                uint8_t *rx_buffer, size_t rx_len, spi_device_t device);

#endif