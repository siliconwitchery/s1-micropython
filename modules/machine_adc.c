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
#include "py/qstr.h"
#include "nrfx_saadc.h"

/**
 * @brief ADC object structure.
 */
typedef struct _machine_adc_obj_t
{
    mp_obj_base_t base;
    uint8_t channel;
    nrf_saadc_input_t p_pin;
    nrf_saadc_input_t n_pin;
    nrf_saadc_resolution_t resolution;
    nrf_saadc_oversample_t oversampling;
    nrf_saadc_resistor_t resistor_p;
    nrf_saadc_resistor_t resistor_n;
    nrf_saadc_gain_t gain;
    nrf_saadc_reference_t reference;
    nrf_saadc_acqtime_t acq_time;
    nrf_saadc_mode_t mode;
} machine_adc_obj_t;

/**
 * @brief Forward declaration of the ADC class object.
 */
const mp_obj_type_t machine_adc_type;

/**
 * @brief Prints info about a perticular ADC object.
 */
STATIC void machine_adc_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    (void)kind;

    // Create a local ADC reference
    machine_adc_obj_t *self = self_in;

    // Assign text for the pin name
    qstr pPin_text;

    switch (self->p_pin)
    {
    case NRF_SAADC_INPUT_AIN2:
        pPin_text = MP_QSTR_PIN_A1;
        break;
    case NRF_SAADC_INPUT_AIN3:
        pPin_text = MP_QSTR_PIN_A2;
        break;
    default:
        break;
    }

    // Get the resolution value
    uint8_t resolution_val;

    switch (self->resolution)
    {
    case NRF_SAADC_RESOLUTION_8BIT:
        resolution_val = 8;
        break;
    case NRF_SAADC_RESOLUTION_10BIT:
        resolution_val = 10;
        break;
    case NRF_SAADC_RESOLUTION_12BIT:
        resolution_val = 12;
        break;
    case NRF_SAADC_RESOLUTION_14BIT:
        resolution_val = 14;
        break;
    }

    // Get the oversampling factor
    uint16_t oversampling_value;

    switch (self->oversampling)
    {
    case NRF_SAADC_OVERSAMPLE_DISABLED:
        oversampling_value = 1;
        break;
    case NRF_SAADC_OVERSAMPLE_2X:
        oversampling_value = 2;
        break;
    case NRF_SAADC_OVERSAMPLE_4X:
        oversampling_value = 4;
        break;
    case NRF_SAADC_OVERSAMPLE_8X:
        oversampling_value = 8;
        break;
    case NRF_SAADC_OVERSAMPLE_16X:
        oversampling_value = 16;
        break;
    case NRF_SAADC_OVERSAMPLE_32X:
        oversampling_value = 32;
        break;
    case NRF_SAADC_OVERSAMPLE_64X:
        oversampling_value = 64;
        break;
    case NRF_SAADC_OVERSAMPLE_128X:
        oversampling_value = 128;
        break;
    case NRF_SAADC_OVERSAMPLE_256X:
        oversampling_value = 256;
        break;
    }

    // Assign text for the positive resistor setting
    qstr pRes_text;

    switch (self->resistor_p)
    {
    case NRF_SAADC_RESISTOR_DISABLED:
        pRes_text = MP_QSTR_PULL_DISABLED;
        break;
    case NRF_SAADC_RESISTOR_PULLDOWN:
        pRes_text = MP_QSTR_PULL_UP;
        break;
    case NRF_SAADC_RESISTOR_PULLUP:
        pRes_text = MP_QSTR_PUUL_DOWN;
        break;
    case NRF_SAADC_RESISTOR_VDD1_2:
        pRes_text = MP_QSTR_PULL_HALF;
        break;
    }

    // Assign text for the negative resistor setting
    qstr nRes_text;

    switch (self->resistor_n)
    {
    case NRF_SAADC_RESISTOR_DISABLED:
        nRes_text = MP_QSTR_PULL_DISABLED;
        break;
    case NRF_SAADC_RESISTOR_PULLDOWN:
        nRes_text = MP_QSTR_PULL_UP;
        break;
    case NRF_SAADC_RESISTOR_PULLUP:
        nRes_text = MP_QSTR_PUUL_DOWN;
        break;
    case NRF_SAADC_RESISTOR_VDD1_2:
        nRes_text = MP_QSTR_PULL_HALF;
        break;
    }

    // Assign text for the gain
    qstr gain_text;

    switch (self->gain)
    {
    case NRF_SAADC_GAIN1_6:
        gain_text = MP_QSTR_GAIN_DIV6;
        break;
    case NRF_SAADC_GAIN1_5:
        gain_text = MP_QSTR_GAIN_DIV5;
        break;
    case NRF_SAADC_GAIN1_4:
        gain_text = MP_QSTR_GAIN_DIV4;
        break;
    case NRF_SAADC_GAIN1_3:
        gain_text = MP_QSTR_GAIN_DIV3;
        break;
    case NRF_SAADC_GAIN1_2:
        gain_text = MP_QSTR_GAIN_DIV2;
        break;
    case NRF_SAADC_GAIN1:
        gain_text = MP_QSTR_GAIN_UNITY;
        break;
    case NRF_SAADC_GAIN2:
        gain_text = MP_QSTR_GAIN_MUL2;
        break;
    case NRF_SAADC_GAIN4:
        gain_text = MP_QSTR_GAIN_MUL4;
        break;
    }

    // Assign text for the reference level
    qstr ref_text;

    switch (self->reference)
    {
    case NRF_SAADC_REFERENCE_INTERNAL:
        ref_text = MP_QSTR_REF_INTERNAL;
        break;
    case NRF_SAADC_REFERENCE_VDD4:
        ref_text = MP_QSTR_REF_QUARTER_VDD;
        break;
    }

    // Get the aquisition time
    uint8_t acq_time_value;

    switch (self->acq_time)
    {
    case NRF_SAADC_ACQTIME_3US:
        acq_time_value = 3;
        break;
    case NRF_SAADC_ACQTIME_5US:
        acq_time_value = 5;
        break;
    case NRF_SAADC_ACQTIME_10US:
        acq_time_value = 10;
        break;
    case NRF_SAADC_ACQTIME_15US:
        acq_time_value = 15;
        break;
    case NRF_SAADC_ACQTIME_20US:
        acq_time_value = 20;
        break;
    case NRF_SAADC_ACQTIME_40US:
        acq_time_value = 40;
        break;
    }

    // Assign text for the mode
    qstr mode_text;

    switch (self->mode)
    {
    case NRF_SAADC_MODE_SINGLE_ENDED:
        mode_text = MP_QSTR_MODE_SINGLE;
        break;
    case NRF_SAADC_MODE_DIFFERENTIAL:
        mode_text = MP_QSTR_MODE_DIFF;
        break;
    }

    // Print all the information about the ADC object
    mp_printf(print, "ADC(ch=%u, pPin=%q, res=%u[bit], samp=%u, pRes=%q, "
                     "nRes=%q, gain=%q, ref=%q, acq=%d[us], mode=%q)",
              self->channel, pPin_text, resolution_val, oversampling_value,
              pRes_text, nRes_text, gain_text, ref_text, acq_time_value,
              mode_text);
}

/**
 * @brief Function which creates a new ADC object. Expects the format as:
 *        machine.ADC(channel, PinNum, ...). There are additional parameters
 *        which can be configured. They are shown in the arguments table below.
 */
STATIC mp_obj_t machine_adc_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    // Create the allowed arguments table
    static const mp_arg_t allowed_args[] = {
        {MP_QSTR_ch, MP_ARG_REQUIRED | MP_ARG_OBJ},
        {MP_QSTR_pPin, MP_ARG_REQUIRED | MP_ARG_OBJ},
        {MP_QSTR_res, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_INT(14)}},
        {MP_QSTR_samp, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_INT(32)}},
        {MP_QSTR_pRes, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_INT(NRF_SAADC_RESISTOR_DISABLED)}},
        {MP_QSTR_nRes, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_INT(NRF_SAADC_RESISTOR_DISABLED)}},
        {MP_QSTR_gain, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_INT(NRF_SAADC_GAIN1_6)}},
        {MP_QSTR_ref, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_INT(NRF_SAADC_REFERENCE_INTERNAL)}},
        {MP_QSTR_acq, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_INT(10)}},
        {MP_QSTR_mode, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_INT(NRF_SAADC_MODE_SINGLE_ENDED)}},
    };

    // Parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // Get and check the channel. Channel 7 is reserved for the battery pin
    uint8_t channel = mp_obj_get_int(args[0].u_obj);
    if (channel > 6)
    {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("channel must be between 0 and 6"));
    }

    // Get and check the pin number
    nrf_saadc_input_t p_pin = mp_obj_get_int(args[1].u_obj);

    if (p_pin != NRF_SAADC_INPUT_AIN2 &&
        p_pin != NRF_SAADC_INPUT_AIN3)
    {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("invalid pin for ADC"));
    }

    // Get and check the resolution
    nrf_saadc_resolution_t resolution;

    switch (mp_obj_get_int(args[2].u_obj))
    {
    case 8:
        resolution = NRF_SAADC_RESOLUTION_8BIT;
        break;
    case 10:
        resolution = NRF_SAADC_RESOLUTION_10BIT;
        break;
    case 12:
        resolution = NRF_SAADC_RESOLUTION_12BIT;
        break;
    case 14:
        resolution = NRF_SAADC_RESOLUTION_14BIT;
        break;
    default:
        mp_raise_ValueError(MP_ERROR_TEXT("invalid value for resolution"));
        break;
    }

    // Get and check the oversampling factor
    nrf_saadc_oversample_t oversampling;

    switch (mp_obj_get_int(args[3].u_obj))
    {
    case 1:
        oversampling = NRF_SAADC_OVERSAMPLE_DISABLED;
        break;
    case 2:
        oversampling = NRF_SAADC_OVERSAMPLE_2X;
        break;
    case 4:
        oversampling = NRF_SAADC_OVERSAMPLE_4X;
        break;
    case 8:
        oversampling = NRF_SAADC_OVERSAMPLE_8X;
        break;
    case 16:
        oversampling = NRF_SAADC_OVERSAMPLE_16X;
        break;
    case 32:
        oversampling = NRF_SAADC_OVERSAMPLE_32X;
        break;
    case 64:
        oversampling = NRF_SAADC_OVERSAMPLE_64X;
        break;
    case 128:
        oversampling = NRF_SAADC_OVERSAMPLE_128X;
        break;
    case 256:
        oversampling = NRF_SAADC_OVERSAMPLE_256X;
        break;
    default:
        mp_raise_ValueError(MP_ERROR_TEXT("invalid oversampling factor"));
        break;
    }

    // Get and check the positive input pull option
    nrf_saadc_resistor_t resistor_p = mp_obj_get_int(args[4].u_obj);

    if (resistor_p != NRF_SAADC_RESISTOR_DISABLED &&
        resistor_p != NRF_SAADC_RESISTOR_PULLDOWN &&
        resistor_p != NRF_SAADC_RESISTOR_PULLUP &&
        resistor_p != NRF_SAADC_RESISTOR_VDD1_2)
    {
        mp_raise_ValueError(MP_ERROR_TEXT("invalid option for positive pull resistor"));
    }

    // Get and check the negative input pull option
    nrf_saadc_resistor_t resistor_n = mp_obj_get_int(args[5].u_obj);

    if (resistor_n != NRF_SAADC_RESISTOR_DISABLED &&
        resistor_n != NRF_SAADC_RESISTOR_PULLDOWN &&
        resistor_n != NRF_SAADC_RESISTOR_PULLUP &&
        resistor_n != NRF_SAADC_RESISTOR_VDD1_2)
    {
        mp_raise_ValueError(MP_ERROR_TEXT("invalid option for negative pull resistor"));
    }

    // Get and check the gain
    nrf_saadc_gain_t gain = mp_obj_get_int(args[6].u_obj);

    if (gain != NRF_SAADC_GAIN1_6 &&
        gain != NRF_SAADC_GAIN1_5 &&
        gain != NRF_SAADC_GAIN1_4 &&
        gain != NRF_SAADC_GAIN1_3 &&
        gain != NRF_SAADC_GAIN1_2 &&
        gain != NRF_SAADC_GAIN1 &&
        gain != NRF_SAADC_GAIN2 &&
        gain != NRF_SAADC_GAIN4)
    {
        mp_raise_ValueError(MP_ERROR_TEXT("invalid option for gain"));
    }

    // Get and check the reference
    nrf_saadc_reference_t reference = mp_obj_get_int(args[7].u_obj);

    if (reference != NRF_SAADC_REFERENCE_INTERNAL &&
        reference != NRF_SAADC_REFERENCE_VDD4)
    {
        mp_raise_ValueError(MP_ERROR_TEXT("invalid option for reference level"));
    }

    // Get and check the aquisition time
    nrf_saadc_acqtime_t acq_time;

    switch (mp_obj_get_int(args[8].u_obj))
    {
    case 3:
        acq_time = NRF_SAADC_ACQTIME_3US;
        break;
    case 5:
        acq_time = NRF_SAADC_ACQTIME_5US;
        break;
    case 10:
        acq_time = NRF_SAADC_ACQTIME_10US;
        break;
    case 15:
        acq_time = NRF_SAADC_ACQTIME_15US;
        break;
    case 20:
        acq_time = NRF_SAADC_ACQTIME_20US;
        break;
    case 40:
        acq_time = NRF_SAADC_ACQTIME_40US;
        break;
    default:
        mp_raise_ValueError(MP_ERROR_TEXT("invalid value for aquisition time"));
        break;
    }

    // Get and check the mode
    nrf_saadc_mode_t mode = mp_obj_get_int(args[9].u_obj);

    if (mode != NRF_SAADC_MODE_SINGLE_ENDED &&
        mode != NRF_SAADC_MODE_DIFFERENTIAL)
    {
        mp_raise_ValueError(MP_ERROR_TEXT("invalid pin mode"));
    }

    // If mode is differential, set the other pin to be the negative input
    nrf_saadc_input_t n_pin = NRF_SAADC_INPUT_DISABLED;

    if (mode == NRF_SAADC_MODE_DIFFERENTIAL)
    {
        if (p_pin == NRF_SAADC_INPUT_AIN2)
        {
            n_pin = NRF_SAADC_INPUT_AIN3;
        }
        else
        {
            n_pin = NRF_SAADC_INPUT_AIN2;
        }
    }

    // Create a new ADC object, and save all of its parameters
    machine_adc_obj_t *self = m_new_obj(machine_adc_obj_t);

    self->base.type = &machine_adc_type;
    self->channel = channel;
    self->p_pin = p_pin;
    self->n_pin = n_pin;
    self->resolution = resolution;
    self->oversampling = oversampling;
    self->resistor_p = resistor_p;
    self->resistor_n = resistor_n;
    self->gain = gain;
    self->reference = reference;
    self->acq_time = acq_time;
    self->mode = mode;

    // Build the SAADC configuration structure
    nrfx_saadc_channel_t config = {
        .channel_config = {
            .resistor_p = resistor_p,
            .resistor_n = resistor_n,
            .gain = gain,
            .reference = reference,
            .acq_time = acq_time,
            .mode = mode,
            .burst = NRF_SAADC_BURST_DISABLED,
        },
        .pin_p = p_pin,
        .pin_n = n_pin,
        .channel_index = channel,
    };

    // Configure the channel
    nrfx_saadc_channel_config(&config);

    // Return the new ADC object
    return MP_OBJ_FROM_PTR(self);
}

/**
 * @brief Gets the ADC value of the object called.
 */
STATIC mp_obj_t machine_adc_call(mp_obj_t self_in, size_t n_args, size_t n_kw, const mp_obj_t *args)
{
    // This function should be called without arguments
    mp_arg_check_num(n_args, n_kw, 0, 0, false);

    // Create local ADC object
    machine_adc_obj_t *self = self_in;

    // Configure the conversion
    nrfx_saadc_simple_mode_set(1 << self->channel,
                               self->resolution,
                               self->oversampling,
                               NULL);

    // Set the buffer
    nrf_saadc_value_t value = 0;

    nrfx_saadc_buffer_set(&value, 1);

    // TODO Why doesn't the conversion work without this?
    __NOP();

    // Start the conversion. Will be blocking
    nrfx_saadc_mode_trigger();

    // Return the value
    return MP_OBJ_NEW_SMALL_INT(value);
}

/**
 * @brief Gets the ADC value, and returns the value as a voltage based on the
 *        ADC settings.
 */
STATIC mp_obj_t machine_adc_voltage(mp_obj_t self_in)
{
    // Get the raw ADC value
    uint32_t value = mp_obj_get_int(machine_adc_call(self_in, 0, 0, NULL));

    // Create local ADC object
    machine_adc_obj_t *self = self_in;

    // Get the maximum value for the current resolution
    uint8_t bits;

    switch (self->resolution)
    {
    case NRF_SAADC_RESOLUTION_8BIT:
        bits = 8;
        break;
    case NRF_SAADC_RESOLUTION_10BIT:
        bits = 10;
        break;
    case NRF_SAADC_RESOLUTION_12BIT:
        bits = 12;
        break;
    case NRF_SAADC_RESOLUTION_14BIT:
        bits = 14;
        break;
    }

    // If in differential mode, reduce one bit
    if (self->mode == NRF_SAADC_MODE_DIFFERENTIAL)
    {
        bits -= 1;
    }

    // Get the maximum value
    uint32_t max_res = 2 << (bits - 1);

    // Get the gain
    float gain;

    switch (self->gain)
    {
    case NRF_SAADC_GAIN1_6:
        gain = 1.0f / 6.0f;
        break;
    case NRF_SAADC_GAIN1_5:
        gain = 1.0f / 5.0f;
        break;
    case NRF_SAADC_GAIN1_4:
        gain = 1.0f / 4.0f;
        break;
    case NRF_SAADC_GAIN1_3:
        gain = 1.0f / 3.0f;
        break;
    case NRF_SAADC_GAIN1_2:
        gain = 1.0f / 2.0f;
        break;
    case NRF_SAADC_GAIN1:
        gain = 1.0f;
        break;
    case NRF_SAADC_GAIN2:
        gain = 2.0f;
        break;
    case NRF_SAADC_GAIN4:
        gain = 4.0f;
        break;
    }

    // Get the reference voltage
    float ref;

    switch (self->reference)
    {
    case NRF_SAADC_REFERENCE_INTERNAL:
        ref = 0.6f;
        break;
    case NRF_SAADC_REFERENCE_VDD4:
        ref = 1.8f / 4.0f;
        break;
    }

    // Convert the ADC value
    float voltage = (ref / gain) / max_res * value;

    // Return the voltage
    return mp_obj_new_float(voltage);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_adc_voltage_obj, machine_adc_voltage);

/**
 * @brief Calibrates the ADC.
 */
STATIC mp_obj_t machine_adc_calibrate(void)
{
    nrfx_saadc_offset_calibrate(NULL);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(machine_adc_calibrate_obj, machine_adc_calibrate);

/**
 * @brief Local class dictionary. Contains all the methods and constants of ADC.
 */
STATIC const mp_rom_map_elem_t machine_adc_locals_dict_table[] = {

    // Class methods
    {MP_ROM_QSTR(MP_QSTR_voltage), MP_ROM_PTR(&machine_adc_voltage_obj)},
    {MP_ROM_QSTR(MP_QSTR_calibrate), MP_ROM_PTR(&machine_adc_calibrate_obj)},

    // Resistor configurations
    {MP_ROM_QSTR(MP_QSTR_PULL_DISABLED), MP_ROM_INT(NRF_SAADC_RESISTOR_DISABLED)},
    {MP_ROM_QSTR(MP_QSTR_PULL_UP), MP_ROM_INT(NRF_SAADC_RESISTOR_PULLDOWN)},
    {MP_ROM_QSTR(MP_QSTR_PUUL_DOWN), MP_ROM_INT(NRF_SAADC_RESISTOR_PULLUP)},
    {MP_ROM_QSTR(MP_QSTR_PULL_HALF), MP_ROM_INT(NRF_SAADC_RESISTOR_VDD1_2)},

    // Gain options
    {MP_ROM_QSTR(MP_QSTR_GAIN_DIV6), MP_ROM_INT(NRF_SAADC_GAIN1_6)},
    {MP_ROM_QSTR(MP_QSTR_GAIN_DIV5), MP_ROM_INT(NRF_SAADC_GAIN1_5)},
    {MP_ROM_QSTR(MP_QSTR_GAIN_DIV4), MP_ROM_INT(NRF_SAADC_GAIN1_4)},
    {MP_ROM_QSTR(MP_QSTR_GAIN_DIV3), MP_ROM_INT(NRF_SAADC_GAIN1_3)},
    {MP_ROM_QSTR(MP_QSTR_GAIN_DIV2), MP_ROM_INT(NRF_SAADC_GAIN1_2)},
    {MP_ROM_QSTR(MP_QSTR_GAIN_UNITY), MP_ROM_INT(NRF_SAADC_GAIN1)},
    {MP_ROM_QSTR(MP_QSTR_GAIN_MUL2), MP_ROM_INT(NRF_SAADC_GAIN2)},
    {MP_ROM_QSTR(MP_QSTR_GAIN_MUL4), MP_ROM_INT(NRF_SAADC_GAIN4)},

    // Reference modes
    {MP_ROM_QSTR(MP_QSTR_REF_INTERNAL), MP_ROM_INT(NRF_SAADC_REFERENCE_INTERNAL)},
    {MP_ROM_QSTR(MP_QSTR_REF_QUARTER_VDD), MP_ROM_INT(NRF_SAADC_REFERENCE_VDD4)},

    // Modes for single or differential ended configuration
    {MP_ROM_QSTR(MP_QSTR_MODE_SINGLE), MP_ROM_INT(NRF_SAADC_MODE_SINGLE_ENDED)},
    {MP_ROM_QSTR(MP_QSTR_MODE_DIFF), MP_ROM_INT(NRF_SAADC_MODE_DIFFERENTIAL)},

    // Both usuable Pin IDs
    {MP_ROM_QSTR(MP_QSTR_PIN_A1), MP_ROM_INT(NRF_SAADC_INPUT_AIN2)},
    {MP_ROM_QSTR(MP_QSTR_PIN_A2), MP_ROM_INT(NRF_SAADC_INPUT_AIN3)},
};
STATIC MP_DEFINE_CONST_DICT(machine_adc_locals_dict, machine_adc_locals_dict_table);

/**
 * @brief Class structure for the ADC object.
 */
const mp_obj_type_t machine_adc_type = {
    .base = {&mp_type_type},
    .name = MP_QSTR_ADC,
    .print = machine_adc_print,
    .make_new = machine_adc_make_new,
    .call = machine_adc_call,
    .locals_dict = (mp_obj_dict_t *)&machine_adc_locals_dict,
};