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
#include "py/runtime.h"
#include "main.h"
#include "nrfx_glue.h"

/**
 * @brief Local flag to indicate if the flash is asleep
 */
static bool flash_asleep = true;

/**
 * @brief Helper function to check if the flash is currently busy during a
 *        write or erase process.
 */
static bool flash_busy(void)
{
    // Read status register
    uint8_t status_reg[1] = {0x05};
    uint8_t status_res[2] = {0};
    spim_tx_rx((uint8_t *)&status_reg, 1, (uint8_t *)&status_res, 2, FLASH);

    if (!(status_res[1] & 0x01))
    {
        return false;
    }

    return true;
}

/**
 * @brief Wakes up the flash from deep sleep.
 */
static void machine_flash_wake(void)
{
    // Wake up the flash using the 9 byte sequence
    uint8_t wake_seq[4] = {0xAB, 0, 0, 0};
    uint8_t wake_res[5] = {0};
    spim_tx_rx((uint8_t *)&wake_seq, 4, (uint8_t *)&wake_res, 5, FLASH);

    // Wait tRES1 to come out of sleep
    NRFX_DELAY_US(3);

    // Enable the reset command
    uint8_t reset_en_cmd = 0x66;
    spim_tx_rx((uint8_t *)&reset_en_cmd, 1, NULL, 0, FLASH);

    // Issue the reset command
    uint8_t reset_cmd = 0x99;
    spim_tx_rx((uint8_t *)&reset_cmd + 1, 1, NULL, 0, FLASH);

    // Wait tRST to fully reset
    NRFX_DELAY_US(30);

    // Mark the flash as awake
    flash_asleep = false;
}

/**
 * @brief Puts the flash into deep sleep.
 */
STATIC mp_obj_t machine_flash_sleep(void)
{
    // Issue the deep sleep command
    uint8_t sleep_cmd = 0xB9;
    spim_tx_rx((uint8_t *)&sleep_cmd, 1, NULL, 0, FLASH);

    // Wait tDB to sleep
    NRFX_DELAY_US(2);

    // Mark the flash as asleep
    flash_asleep = true;

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(machine_flash_sleep_obj, machine_flash_sleep);

/**
 * @brief Erases the entire flash if no block number is given. Otherwise erases
 *        the the 4k block provided. Automatically wakes up the flash if needed.
 */
STATIC mp_obj_t machine_flash_erase(size_t n_args, const mp_obj_t *args)
{
    // If flash is asleep, wake it up first
    if (flash_asleep)
    {
        machine_flash_wake();
    }

    // An erase sequence always starts with a write enable instruction
    uint8_t write_enable_cmd = 0x06;
    spim_tx_rx((uint8_t *)&write_enable_cmd, 1, NULL, 0, FLASH);

    // If no args are given
    if (n_args == 0)
    {
        // Issue the chip erase command
        uint8_t chip_erase_cmd = 0x60;
        spim_tx_rx((uint8_t *)&chip_erase_cmd, 1, NULL, 0, FLASH);

        // Wait until the erase is complete
        while (flash_busy())
        {
            // Wait 1ms to check again
            NRFX_DELAY_US(1000);
        }

        return mp_const_none;
    }

    // Otherwise, ensure block is in a valid range
    if (mp_obj_get_int(args[0]) > 1023)
    {
        mp_raise_ValueError(MP_ERROR_TEXT("block number must be less than 1024"));
    }

    // Determine address from the page number
    uint32_t address = mp_obj_get_int(args[0]) * 0x1000;

    // Erase a 4k page from the 24bit address given
    uint8_t erase_block[4] = {
        0x20,
        (uint8_t)(address >> 16),
        (uint8_t)(address >> 8),
        0x00 // Bottom byte is always 0
    };
    spim_tx_rx((uint8_t *)&erase_block, 4, NULL, 0, FLASH);

    // Wait until the erase is complete
    while (flash_busy())
    {
        // Wait 1ms to check again
        NRFX_DELAY_US(1000);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_flash_erase_obj, 0, 1, machine_flash_erase);

/**
 * @brief Reads n bytes from a page of Flash, where n is the length of the read
 *        buffer. n cannot be bigger than 256 bytes. Automatically wakes up the
 *        flash if needed.
 * @param page: The 256 byte page to read from.
 * @param read_obj: The read buffer as a bytearray() object.
 */
STATIC mp_obj_t machine_flash_read(mp_obj_t page, mp_obj_t read_obj)
{
    // Create a read buffer from the object given
    mp_buffer_info_t read;
    mp_get_buffer_raise(read_obj, &read, MP_BUFFER_WRITE);

    // Check page size
    if (read.len > 256)
    {
        mp_raise_ValueError(MP_ERROR_TEXT("buffer cannot be bigger than 256 bytes"));
    }

    // If flash is asleep, wake it up first
    if (flash_asleep)
    {
        machine_flash_wake();
    }

    // Get the address from the page give
    uint32_t address = mp_obj_get_int(page) * 0x100;

    // Prepare the read command along with address
    uint8_t read_cmd[4] = {
        0x03,
        (uint8_t)(address >> 16),
        (uint8_t)(address >> 8),
        0x00, // Bottom byte is always 0
    };

    // Create an rx buffer big enough for the read sequence and payload
    uint8_t read_buff[4 + 256];

    // Send read sequence, and read data into the temporary buffer
    spim_tx_rx((uint8_t *)&read_cmd, 4, read_buff, read.len + 4, FLASH);

    // Copy the data from the temporary buffer into the real one
    memcpy(read.buf, read_buff + 4, read.len);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(machine_flash_read_obj, machine_flash_read);

/**
 * @brief Writes n bytes from a page of Flash, where n is the length of the
 *        write buffer. n cannot be bigger than 256 bytes. Automatically wakes
 *        up the flash if needed.
 * @param page: The 256 byte page to write to.
 * @param write_obj: The write buffer as a bytearray() object.
 */
STATIC mp_obj_t machine_flash_write(mp_obj_t page, mp_obj_t write_obj)
{
    // Create a write buffer from the object given
    mp_buffer_info_t write;
    mp_get_buffer_raise(write_obj, &write, MP_BUFFER_READ);

    // Check page size
    if (write.len > 256)
    {
        mp_raise_ValueError(MP_ERROR_TEXT("buffer cannot be bigger than 256 bytes"));
    }

    // If flash is asleep, wake it up first
    if (flash_asleep)
    {
        machine_flash_wake();
    }

    // Write sequence always starts with a write enable instruction
    uint8_t write_enable_cmd = 0x06;
    spim_tx_rx((uint8_t *)&write_enable_cmd, 1, NULL, 0, FLASH);

    // Get the address from the page give
    uint32_t address = mp_obj_get_int(page) * 0x100;

    // Create a tx buffer big enough for the write sequence and payload
    uint8_t write_buff[4 + 256];

    // Populate the write sequence
    write_buff[0] = 0x02;
    write_buff[1] = (uint8_t)(address >> 16);
    write_buff[2] = (uint8_t)(address >> 8);
    write_buff[3] = 0x00; // Bottom byte is always 0

    // Copy the payload into the write buffer
    memcpy(write_buff + 4, write.buf, write.len);

    // Send the data
    spim_tx_rx((uint8_t *)&write_buff, write.len + 4, NULL, 0, FLASH);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(machine_flash_write_obj, machine_flash_write);

/**
 * @brief Global module dictionary containing all of the methods and constants
 *        for the flash module.
 */
STATIC const mp_rom_map_elem_t machine_flash_locals_dict_table[] = {

    // Local methods
    {MP_ROM_QSTR(MP_QSTR_sleep), MP_ROM_PTR(&machine_flash_sleep_obj)},
    {MP_ROM_QSTR(MP_QSTR_erase), MP_ROM_PTR(&machine_flash_erase_obj)},
    {MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&machine_flash_read_obj)},
    {MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&machine_flash_write_obj)},
};
STATIC MP_DEFINE_CONST_DICT(machine_flash_locals_dict, machine_flash_locals_dict_table);

/**
 * @brief Module structure for the FLASH object.
 */
const mp_obj_type_t machine_flash_type = {
    .base = {&mp_type_type},
    .name = MP_QSTR_Flash,
    .print = NULL,
    .make_new = NULL,
    .call = NULL,
    .locals_dict = (mp_obj_dict_t *)&machine_flash_locals_dict,
};