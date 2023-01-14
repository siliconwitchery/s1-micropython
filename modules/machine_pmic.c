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

#include "modmachine.h"
#include "main.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "nrf.h"
#include "nrfx_saadc.h"
#include "nrfx_twim.h"
#include "math.h"

/**
 * @brief Instance for the I2C controller driver.
 */
static const nrfx_twim_t i2c_instance = NRFX_TWIM_INSTANCE(0);

/**
 * @brief The PMIC I2C address.
 */
#define PMIC_I2C_ADDRESS 0x48

/**
 * @brief The analog pin for battery monitoring
 */
#define PMIC_AMUX_PIN NRF_SAADC_INPUT_AIN1

/**
 * @brief Local function to read a PMIC register.
 */
static uint8_t read_reg(uint8_t reg)
{
    // Receive buffer
    uint8_t rx_buffer;

    // Create a descriptor for a 1 byte write, and 1 byte read
    nrfx_twim_xfer_desc_t i2c_xfer =
        NRFX_TWIM_XFER_DESC_TXRX(PMIC_I2C_ADDRESS, &reg, 1, &rx_buffer, 1);

    // Start a transfer
    nrfx_err_t err = nrfx_twim_xfer(&i2c_instance, &i2c_xfer, 0);

    // Catch errors. Only on lower 2 bytes due to NRFX_ERROR_BASE_NUM
    assert_if(0x0000FFFF & err);

    // Return the received byte
    return rx_buffer;
}

/**
 * @brief Local function to write to a PMIC register.
 */
static void write_reg(uint8_t reg, uint8_t value)
{
    // Transmit buffer
    uint8_t buffer[2];

    // Create a descriptor for a 2 byte write
    nrfx_twim_xfer_desc_t i2c_xfer =
        NRFX_TWIM_XFER_DESC_TX(PMIC_I2C_ADDRESS, buffer, 2);

    // Fill the buffer with the register address, and the value to set
    buffer[0] = reg;
    buffer[1] = value;

    // Start a transfer
    nrfx_err_t err = nrfx_twim_xfer(&i2c_instance, &i2c_xfer, 0);

    // Catch errors. Only on lower 2 bytes due to NRFX_ERROR_BASE_NUM
    assert_if(err);
}

/**
 * @brief Initialises the PMIC module.
 */
void machine_pmic_init(void)
{
    // Initialise the I2C bus to the PMIC
    nrfx_twim_config_t i2c_conf = {
        .scl = 17,
        .sda = 14,
        .frequency = NRF_TWIM_FREQ_400K,
        .interrupt_priority = NRFX_TWIM_DEFAULT_CONFIG_IRQ_PRIORITY,
        .hold_bus_uninit = false,
    };
    nrfx_twim_init(&i2c_instance, &i2c_conf, NULL, NULL);

    // Enable the I2C
    nrfx_twim_enable(&i2c_instance);

    // Check PMIC Chip ID
    if (read_reg(0x14) != 0x7A)
    {
        assert_if(1); // TODO add a special code here to return?
    }

    // Build the SAADC configuration structure for the battery voltage pin
    nrfx_saadc_channel_t adc_conf = {
        .channel_config = {
            .resistor_p = NRF_SAADC_RESISTOR_DISABLED,
            .resistor_n = NRF_SAADC_RESISTOR_DISABLED,
            .gain = NRF_SAADC_GAIN1_3,
            .reference = NRF_SAADC_REFERENCE_INTERNAL,
            .acq_time = NRF_SAADC_ACQTIME_40US, // TODO fine tune this
            .mode = NRF_SAADC_MODE_SINGLE_ENDED,
            .burst = NRF_SAADC_BURST_DISABLED,
        },
        .pin_p = NRF_SAADC_INPUT_AIN1,
        .pin_n = NRF_SAADC_INPUT_DISABLED,
        .channel_index = 7,
    };

    // Configure the channel
    nrfx_saadc_channel_config(&adc_conf);
}

/**
 * @brief Configures the Li-Po charge voltage and/or current using keyword args
 *        v and i. If no args are given the current settings are printed.
 */
STATIC mp_obj_t pmic_charge_config(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    // Create the allowed arguments table
    static const mp_arg_t allowed_args[] = {
        {MP_QSTR_v, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE}},
        {MP_QSTR_i, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE}},
    };

    // Parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // If no args are provided
    if (args[0].u_obj == mp_const_none && args[1].u_obj == mp_const_none)
    {
        // Print the current value

        // Read the charge voltage register (top 6 bits)
        uint8_t reg_value = read_reg(0x26) >> 2;

        // Convert the register value to a voltage
        float voltage = (reg_value * 0.025f) + 3.6f;

        // Read the charge current register (top 6 bits)
        reg_value = read_reg(0x24) >> 2;

        // Convert the register value to a current (mA)
        float current = (reg_value * 7.5f) + 7.5f;

        // Put both values into a tuple
        mp_obj_t tuple[2] = {
            tuple[0] = mp_obj_new_float(voltage),
            tuple[1] = mp_obj_new_float(current),
        };

        // Return the values
        return mp_obj_new_tuple(2, tuple);
    }

    // If voltage setting is provided
    if (args[0].u_obj != mp_const_none)
    {
        float voltage = mp_obj_get_float(args[0].u_obj);

        // Check if voltage is a valid range
        // TODO by default, Vsys isn't high enough to allow more than 4.3V
        if (voltage < 3.6f || voltage > 4.3f)
        {
            mp_raise_ValueError(MP_ERROR_TEXT("charge voltage must be between 3.6V and 4.3V"));
        }

        // Set the charging voltage (shifted to be in the top 6 bits of the register)
        uint8_t voltage_setting = (uint8_t)round((voltage - 3.6f) / 0.025f) << 2;

        // Apply the voltage, and ensure charging is allowed
        write_reg(0x26, voltage_setting | 0b00);
    }

    // If current setting is provided
    if (args[1].u_obj != mp_const_none)
    {
        float current = mp_obj_get_float(args[1].u_obj);

        // Check if the current is a valid range
        if (current < 7.5f || current > 300.0f)
        {
            mp_raise_ValueError(MP_ERROR_TEXT("charge current must be between 7.5mA and 300mA"));
        }

        // Set the charging current (shifted to be in the top 6 bits of the register)
        uint8_t current_setting = (uint8_t)round((current - 7.5f) / 7.5f) << 2;

        // Apply the current, and ensure a 3hr safety timer is set
        write_reg(0x24, current_setting | 0b01);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pmic_charge_config_obj, 0, pmic_charge_config);

/**
 * @brief Enables or disables the FPGA core power. If no arguments are given,
 *        the current setting is printed.
 */
STATIC mp_obj_t pmic_fpga_power(size_t n_args, const mp_obj_t *args)
{
    // If no args are given, print out the current state of the SBB1 register
    if (n_args == 0)
    {
        // Read the SBB1 register
        uint8_t sbb1_reg = read_reg(0x2C);

        // Return the value. Second bit represents on or off
        return mp_obj_new_bool(sbb1_reg & 0b10);
    }

    // Otherwise extract the enable state from the first argument
    bool enable = mp_obj_get_int(args[0]);

    // Ensure SBB1 is 1.2V (TODO later we can do undervolting tricks)
    write_reg(0x2B, 0x08);

    // If enable
    if (enable)
    {
        // Enable SBB1 as buck mode with 0.333A limit
        write_reg(0x2C, 0x7E);

        return mp_const_none;
    }

    // Otherwise, first disable LDO0 (Vio) to avoid damaging the FPGA
    write_reg(0x39, 0x0C);

    // Finally, disable SBB1
    write_reg(0x2C, 0x7C);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pmic_fpga_power_obj, 0, 1, pmic_fpga_power);

/**
 * @brief Configures the Vaux output voltage. If no arguments are given, the
 *        current setting is printed.
 */
STATIC mp_obj_t pmic_vaux_config(size_t n_args, const mp_obj_t *args)
{
    // If no argument is given, print the current setting
    if (n_args == 0)
    {
        // Check if SBB2 is enabled
        bool vaux_en = (read_reg(0x2E) & 0b110) == 0b110;

        // If SBB2 is off, return 0V
        if (vaux_en == false)
        {
            return MP_OBJ_NEW_SMALL_INT(0);
        }

        // Otherwise read the current set value (mask 7 bits)
        uint8_t reg_value = read_reg(0x2D) & 0x7F;

        // Convert the register value to a voltage
        float set_voltage = (reg_value * 0.05f) + 0.8f;

        // Return the voltage
        return mp_obj_new_float(set_voltage);
    }

    // Otherwise, get the desired voltage from the first argument
    float voltage = mp_obj_get_float(args[0]);

    // If 0V, shutdown SBB2
    if (voltage == 0.0f)
    {
        write_reg(0x2E, 0x0C);
        return mp_const_none;
    }

    // Disallow voltage settings outside of the normal range
    if (voltage < 0.8f || voltage > 5.5f)
    {
        mp_raise_ValueError(MP_ERROR_TEXT("Vaux can only be set to 0V, or between 0.8V and 5.5V"));
    }

    // If voltage > than 3.45
    if (voltage > 3.45f)
    {
        // Then LDO0 must not be in LSW mode, otherwise it'll blow up the FPGA
        if ((read_reg(0x39) & 0x10) == 0x10)
        {
            mp_raise_ValueError(MP_ERROR_TEXT("Vaux cannot exceed 3.45V when Vio is in LSW mode"));
        }
    }

    // Set the SBB2 target voltage
    write_reg(0x2D, (uint8_t)round((voltage - 0.8f) / 0.05f));

    // Enable SBB2 as buck-boost, with 1A limit, and discharge resistor enabled
    write_reg(0x2E, 0x0E);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pmic_vaux_config_obj, 0, 1, pmic_vaux_config);

/**
 * @brief Prints the current battery level. If true, or false is given as an
 *        argument, the measuring can be enabled or disabled to save power.
 */
STATIC mp_obj_t pmic_battery_level(size_t n_args, const mp_obj_t *args)
{
    // If no args are give, read the ADC pin
    if (n_args == 0)
    {
        // Check if the AMUX is enabled
        bool battery_adc_enabled = read_reg(0x28) & 0x03;

        // If battery ADC is not enabled, return an error
        if (!battery_adc_enabled)
        {
            mp_raise_ValueError(MP_ERROR_TEXT("battery measurement not enabled"));
        }

        // Otherwise start by configuring a new conversion
        nrfx_saadc_simple_mode_set(7,
                                   NRF_SAADC_RESOLUTION_14BIT,
                                   NRF_SAADC_OVERSAMPLE_16X, // TODO optimize
                                   NULL);

        // Create a buffer
        nrf_saadc_value_t value;
        nrfx_saadc_buffer_set(&value, 1);

        // Start the conversion. Will be blocking
        __NOP(); // TODO Why doesn't the conversion work without this?
        nrfx_saadc_mode_trigger();

        // Convert the ADC value. 0.6v ref, gain is 1/3, resolution is 14bits
        float voltage = (0.6f / (1.0f / 3.0f)) / 16384 * value;

        // Normalise the value for the AMUX range 0V - 1.25V. Gain = 0.272
        voltage /= 0.272f;

        // Return the value
        return mp_obj_new_float(voltage);
    }

    // Otherwise extract the enable state from the first argument
    bool enable = mp_obj_get_int(args[0]);

    // If enable
    if (enable)
    {
        // Configure the PMIC to enable the battery measurement
        write_reg(0x28, 0xF3);

        return mp_const_none;
    }

    // Otherwise disable the measurement
    write_reg(0x28, 0xF0);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pmic_battery_level_obj, 0, 1, pmic_battery_level);

/**
 * @brief Configures the Vio output voltage. If no arguments are given, the
 *        current setting is printed. If lsw is provided as an argument, LDO0
 *        will be switched into load switch mode.
 */
STATIC mp_obj_t pmic_vio_config(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    // Create the allowed arguments table
    static const mp_arg_t allowed_args[] = {
        {MP_QSTR_value, MP_ARG_OBJ},
        {MP_QSTR_lsw, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE}},
    };

    // Parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // If SBB2 is disabled, notify the user
    if ((read_reg(0x2E) & 0b110) != 0b110)
    {
        mp_printf(&mp_plat_print, "Vaux is not enabled. Vio will not be powered\n");
    }

    // If no arguments are given, print the current setting
    if (n_args == 0 && args[1].u_obj == mp_const_none)
    {
        // Check if in LSW mode
        if ((read_reg(0x39) & 0x10) == 0x10)
        {
            // Check if regulator is enabled
            if ((read_reg(0x39) & 0b110) == 0b110)
            {
                // Return flag
                return MP_OBJ_NEW_QSTR(MP_QSTR_LOAD_SWITCH_ON);
            }

            // Otherwise return regulator disabled flag
            return MP_OBJ_NEW_QSTR(MP_QSTR_LOAD_SWITCH_OFF);
        }

        // Otherwise check if LDO0 is enabled
        if ((read_reg(0x39) & 0b110) == 0b110)
        {
            // Check LDO0 set voltage (mask 7 bits)
            uint8_t reg_value = read_reg(0x38) & 0x7F;

            // Convert the register value into a voltage
            float ldo_voltage = (reg_value * 0.025f) + 0.8f;

            // Check SBB2 (Vaux) set voltage (mask 7 bits)
            reg_value = read_reg(0x2D) & 0x7F;

            // Convert the register value into a voltage
            float sbb2_voltage = (reg_value * 0.05f) + 0.8f;

            // If sbb2 voltage is too low (including the 100mV dropout)
            if (sbb2_voltage < ldo_voltage + 0.1f)
            {
                // Notify the user
                mp_printf(&mp_plat_print, "Vaux set too low. Voltage must be 100mV above Vio for correct regulation\n");
            }

            // Return the converted voltage
            return mp_obj_new_float(ldo_voltage);
        }

        // Otherwise LDO0 is 0V
        return mp_obj_new_int(0);
    }

    // Check if fpga is powered off
    if ((read_reg(0x2C) & 0b10) == 0)
    {
        // Prevent configuration if FPGA core rail is off
        mp_raise_ValueError(MP_ERROR_TEXT("Vio cannot be configured while FPGA is powered down"));
    }

    // If the lsw flag was provided
    if (args[1].u_obj != mp_const_none)
    {
        // If lsw=True
        if (mp_obj_get_int(args[1].u_obj))
        {
            // Read SBB2 to ensure it's not above the 3.45V limit of the FPGA IO
            // reg_value = (3.45 - 0.8) / 0.05 = 53
            if ((read_reg(0x2D) & 0x7F) > 53)
            {
                mp_raise_ValueError(MP_ERROR_TEXT("Vaux cannot exceed 3.45V when Vio is in LSW mode"));
            }

            // If the first argument it also trye
            if (mp_obj_get_int(args[0].u_obj))
            {
                // Turn on the regulator with LSW mode with discharge enabled
                write_reg(0x39, 0x1E);

                // Return
                return mp_const_none;
            }

            // Turn off the regulator with LSW mode with discharge enabled
            write_reg(0x39, 0x1C);

            // Return
            return mp_const_none;
        }
    }

    // Get the float value of the first argument
    float ldo_voltage = mp_obj_get_float(args[0].u_obj);

    // If user requests 0V
    if (ldo_voltage == 0.0f)
    {
        // Turn off the regulator, ensuring LDO mode and discharge resistor is set
        write_reg(0x39, 0x0C);

        // Return
        return mp_const_none;
    }

    // Disallow voltage settings outside of the normal range
    if (ldo_voltage < 0.8f || ldo_voltage > 3.45f)
    {
        mp_raise_ValueError(MP_ERROR_TEXT("Vio can only be set to 0V, or between 0.8V and 3.45V"));
    }

    // Otherwise, check SBB2 (Vaux) set voltage (mask 7 bits)
    uint8_t reg_value = read_reg(0x2D) & 0x7F;

    // Convert the register value into a voltage
    float sbb2_voltage = (reg_value * 0.05f) + 0.8f;

    // If sbb2 voltage is too low (including the 100mV dropout)
    if (sbb2_voltage < ldo_voltage + 0.1f)
    {
        // Notify the user
        mp_printf(&mp_plat_print, "Vaux set too low. Voltage must be 100mV above Vio for correct regulation\n");
    }

    // Set the output voltage
    write_reg(0x38, (uint8_t)round((ldo_voltage - 0.8f) / 0.025f));

    // Turn on the regulator with LDO mode set, and discharge enabled
    write_reg(0x39, 0x0E);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pmic_vio_config_obj, 0, pmic_vio_config);

/**
 * @brief Global module dictionary containing all of the methods and constants
 *        for the pmic module.
 */
STATIC const mp_rom_map_elem_t machine_pmic_locals_dict_table[] = {

    // Class methods
    {MP_ROM_QSTR(MP_QSTR_charge_config), MP_ROM_PTR(&pmic_charge_config_obj)},
    {MP_ROM_QSTR(MP_QSTR_fpga_power), MP_ROM_PTR(&pmic_fpga_power_obj)},
    {MP_ROM_QSTR(MP_QSTR_vaux_config), MP_ROM_PTR(&pmic_vaux_config_obj)},
    {MP_ROM_QSTR(MP_QSTR_battery_level), MP_ROM_PTR(&pmic_battery_level_obj)},
    {MP_ROM_QSTR(MP_QSTR_vio_config), MP_ROM_PTR(&pmic_vio_config_obj)},
    // TODO Do we need the PMIC shutdown feature?
};
STATIC MP_DEFINE_CONST_DICT(machine_pmic_locals_dict, machine_pmic_locals_dict_table);

/**
 * @brief Module structure for the PMIC object.
 */
const mp_obj_type_t machine_pmic_type = {
    .base = {&mp_type_type},
    .name = MP_QSTR_PMIC,
    .print = NULL,
    .make_new = NULL,
    .call = NULL,
    .locals_dict = (mp_obj_dict_t *)&machine_pmic_locals_dict,
};