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
#include "py/objstr.h"
#include "py/objtuple.h"
#include "genhdr/mpversion.h"
#include "modmachine.h"
#include "nrf.h"
#include "nrf_soc.h"
#include "ble_gap.h"

/**
 * @brief Micropython system version as a tuple.
 */
STATIC const mp_obj_tuple_t mp_machine_version_info_obj = {
    {&mp_type_tuple},
    3,
    {MP_OBJ_NEW_SMALL_INT(MICROPY_VERSION_MAJOR),
     MP_OBJ_NEW_SMALL_INT(MICROPY_VERSION_MINOR),
     MP_OBJ_NEW_SMALL_INT(MICROPY_VERSION_MICRO)}};

/**
 * @brief Current bit tag as a string object.
 */
STATIC const MP_DEFINE_STR_OBJ(mp_machine_git_tag_obj, MICROPY_GIT_TAG);

/**
 * @brief Current build date as a string object.
 */
STATIC const MP_DEFINE_STR_OBJ(mp_machine_build_date_obj, MICROPY_BUILD_DATE);

/**
 * @brief Board name as a string object.
 */
STATIC const MP_DEFINE_STR_OBJ(mp_machine_board_name_obj, MICROPY_HW_BOARD_NAME);

/**
 * @brief MCU name as a string object.
 */
STATIC const MP_DEFINE_STR_OBJ(mp_machine_mcu_name_obj, MICROPY_HW_MCU_NAME);

/**
 * @brief Prints out the 48 bit device MAC address.
 */
STATIC mp_obj_t machine_mac_address(void)
{
    // Gets the 48bit device MAC address
    ble_gap_addr_t mac;
    sd_ble_gap_addr_get(&mac);

    // Copy the mac address into a string as a 12 character hex
    char mac_str[] = "XXXXXXXXXXXX";

    uint8_t str_pos = 11, glyph;
    uint64_t quotent = (uint64_t)mac.addr[5] << 40 |
                       (uint64_t)mac.addr[4] << 32 |
                       (uint64_t)mac.addr[3] << 24 |
                       (uint64_t)mac.addr[2] << 16 |
                       (uint64_t)mac.addr[1] << 8 |
                       (uint64_t)mac.addr[0];

    while (quotent != 0)
    {
        glyph = quotent % 16;
        glyph < 10 ? (glyph += 48) : (glyph += 55);
        mac_str[str_pos--] = glyph;
        quotent /= 16;
    }

    return mp_obj_new_str(mac_str, 12);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(machine_mac_address_obj, machine_mac_address);

/**
 * @brief Resets the device.
 */
STATIC mp_obj_t machine_reset(void)
{
    NVIC_SystemReset();

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(machine_reset_obj, machine_reset);

/**
 * @brief Prints the last reset cause and clears the RESETREAS register.
 */
STATIC mp_obj_t machine_reset_cause(void)
{
    // Get the reset reason
    uint32_t reset_reason = NRF_POWER->RESETREAS;

    // Decode the reason from the bit set
    qstr reset_reason_text = MP_QSTR_RESET_CAUSE_NONE;

    if (reset_reason & POWER_RESETREAS_SREQ_Msk)
    {
        reset_reason_text = MP_QSTR_RESET_CAUSE_SOFT;
    }
    else if (reset_reason & POWER_RESETREAS_LOCKUP_Msk)
    {
        reset_reason_text = MP_QSTR_RESET_CAUSE_LOCKUP;
    }
    else if (reset_reason & POWER_RESETREAS_OFF_Msk)
    {
        reset_reason_text = MP_QSTR_RESET_CAUSE_GPIO_WAKE;
    }

    // Clear the reset reason register
    NRF_POWER->RESETREAS = 0xFFFFFFFF;

    // Return the reset reason
    return MP_OBJ_NEW_QSTR(reset_reason_text);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(machine_reset_cause_obj, machine_reset_cause);

/**
 * @brief Puts the nRF into system off mode. Only pin resets, or GPIO interrupts
 *        will wake up and reset the device.
 */
STATIC mp_obj_t machine_power_down(void)
{
    sd_power_system_off();
    // Should never return
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(machine_power_down_obj, machine_power_down);

/**
 * @brief Global module dictionary containing all of the methods, constants and
 *        classes for the machine module.
 */
STATIC const mp_rom_map_elem_t machine_module_globals_table[] = {

    {MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_machine)},

    // Local methods
    {MP_ROM_QSTR(MP_QSTR_mac_address), MP_ROM_PTR(&machine_mac_address_obj)},
    {MP_ROM_QSTR(MP_QSTR_reset), MP_ROM_PTR(&machine_reset_obj)},
    {MP_ROM_QSTR(MP_QSTR_reset_cause), MP_ROM_PTR(&machine_reset_cause_obj)},
    {MP_ROM_QSTR(MP_QSTR_power_down), MP_ROM_PTR(&machine_power_down_obj)},
    // {MP_ROM_QSTR(MP_QSTR_bootloader), MP_ROM_PTR(&machine_bootloader_obj)},

    // Classes for the hardware peripherals
    {MP_ROM_QSTR(MP_QSTR_ADC), MP_ROM_PTR(&machine_adc_type)},
    {MP_ROM_QSTR(MP_QSTR_Flash), MP_ROM_PTR(&machine_flash_type)},
    {MP_ROM_QSTR(MP_QSTR_FPGA), MP_ROM_PTR(&machine_fpga_type)},
    {MP_ROM_QSTR(MP_QSTR_PMIC), MP_ROM_PTR(&machine_pmic_type)},
    {MP_ROM_QSTR(MP_QSTR_Pin), MP_ROM_PTR(&machine_pin_type)},
    {MP_ROM_QSTR(MP_QSTR_RTC), MP_ROM_PTR(&machine_rtc_type)},

    // TODO Some extra features we can add later if there's space
    // {MP_ROM_QSTR(MP_QSTR_Temp), MP_ROM_PTR(&machine_temp_type)},
    // {MP_ROM_QSTR(MP_QSTR_RNG), MP_ROM_PTR(&machine_rng_type)},

    // Information about the version and device
    {MP_ROM_QSTR(MP_QSTR_version), MP_ROM_PTR(&mp_machine_version_info_obj)},
    {MP_ROM_QSTR(MP_QSTR_git_tag), MP_ROM_PTR(&mp_machine_git_tag_obj)},
    {MP_ROM_QSTR(MP_QSTR_build_date), MP_ROM_PTR(&mp_machine_build_date_obj)},
    {MP_ROM_QSTR(MP_QSTR_board_name), MP_ROM_PTR(&mp_machine_board_name_obj)},
    {MP_ROM_QSTR(MP_QSTR_mcu_name), MP_ROM_PTR(&mp_machine_mcu_name_obj)},
};
STATIC MP_DEFINE_CONST_DICT(machine_module_globals, machine_module_globals_table);

/**
 * @brief Module structure for the machine object.
 */
const mp_obj_module_t machine_module = {
    .base = {&mp_type_module},
    .globals = (mp_obj_dict_t *)&machine_module_globals,
};

/**
 * @brief Registration of the machine module.
 */
MP_REGISTER_MODULE(MP_QSTR_machine, machine_module);