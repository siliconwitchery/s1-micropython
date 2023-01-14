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
#include "py/qstr.h"
#include "nrfx_gpiote.h"

/**
 * @brief Pin object structure.
 */
typedef struct _machine_pin_obj_t
{
    mp_obj_base_t base;
    uint32_t pin;
    mp_obj_t irq_handler;
} machine_pin_obj_t;

/**
 * @brief Forward declaration of the Pin class object.
 */
const mp_obj_type_t machine_pin_type;

/**
 * @brief Pin IRQ handler.
 */
void pin_irq_handler(nrfx_gpiote_pin_t pin, nrfx_gpiote_trigger_t trigger, void *p_context)
{
    // Get the pin object from the context pointer
    machine_pin_obj_t *self = (machine_pin_obj_t *)p_context;

    // Issue the callback and pass the pin number
    mp_call_function_0(self->irq_handler);
}

/**
 * @brief Prints info about a perticular Pin object.
 */
STATIC void machine_pin_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    (void)kind;

    machine_pin_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // Get the pin mode
    nrf_gpio_pin_dir_t mode = nrf_gpio_pin_dir_get(self->pin);

    // Get the pull direction
    nrf_gpio_pin_pull_t pull = nrf_gpio_pin_pull_get(self->pin);

    // Assign text for the pin name
    qstr pin_text;

    switch (self->pin)
    {
    case 4:
        pin_text = MP_QSTR_PIN_A1;
        break;
    case 5:
        pin_text = MP_QSTR_PIN_A2;
        break;
    }

    // Assign text for pin mode
    qstr mode_text;

    switch (mode)
    {
    case NRF_GPIO_PIN_DIR_INPUT:
        mode_text = MP_QSTR_IN;
        break;
    case NRF_GPIO_PIN_DIR_OUTPUT:
        mode_text = MP_QSTR_OUT;
        break;
    }

    // Assign text for pull direction
    qstr pull_text;

    switch (pull)
    {
    case NRF_GPIO_PIN_NOPULL:
        pull_text = MP_QSTR_PULL_DISABLED;
        break;
    case NRF_GPIO_PIN_PULLDOWN:
        pull_text = MP_QSTR_PULL_DOWN;
        break;
    case NRF_GPIO_PIN_PULLUP:
        pull_text = MP_QSTR_PULL_UP;
        break;
    }

    // Print everything about the pin
    mp_printf(print, "Pin(%q, mode=%q, pull=%q)", pin_text, mode_text, pull_text);
}

/**
 * @brief Function which creates a new Pin object. Expects the format as:
 *        machine.Pin(PinNum, mode=PinMode, pull=PinPull, value=PinValue),
 *        where mode, pull and value are optional keyword arguments.
 */
STATIC mp_obj_t machine_pin_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    // Create the allowed arguments table
    static const mp_arg_t allowed_args[] = {
        {MP_QSTR_pin, MP_ARG_REQUIRED | MP_ARG_OBJ},
        {MP_QSTR_mode, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_INT(NRF_GPIO_PIN_DIR_INPUT)}},
        {MP_QSTR_pull, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_INT(NRF_GPIO_PIN_NOPULL)}},
        {MP_QSTR_drive, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_INT(NRF_GPIO_PIN_S0S1)}},
    };

    // Parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // Get the pin number from the first argument
    int32_t pin = mp_obj_get_int(args[0].u_obj);

    // Get the pin mode from the second argument
    nrf_gpio_pin_dir_t mode = mp_obj_get_int(args[1].u_obj);

    // Get the pin pull direction from the third argument
    nrf_gpio_pin_pull_t pull = mp_obj_get_int(args[2].u_obj);

    // Get the drive mode from the fourth argument
    nrf_gpio_pin_drive_t drive = mp_obj_get_int(args[3].u_obj);

    // If the pin doesn't exist, throw an error
    if (pin != 4 &&
        pin != 5)
    {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("pin %d doesn't exist"), pin);
    }

    // If the mode is invalid, thrown an error
    if (mode != NRF_GPIO_PIN_DIR_INPUT &&
        mode != NRF_GPIO_PIN_DIR_OUTPUT)
    {
        mp_raise_ValueError(MP_ERROR_TEXT("invalid pin mode"));
    }

    // If the pull direction is invalid, thrown an error
    if (pull != NRF_GPIO_PIN_NOPULL &&
        pull != NRF_GPIO_PIN_PULLDOWN &&
        pull != NRF_GPIO_PIN_PULLUP)
    {
        mp_raise_ValueError(MP_ERROR_TEXT("invalid pin pull direction"));
    }

    // If the drive mode is invalid, thrown an error
    if (drive != NRF_GPIO_PIN_S0S1 &&
        drive != NRF_GPIO_PIN_H0S1 &&
        drive != NRF_GPIO_PIN_S0H1 &&
        drive != NRF_GPIO_PIN_H0H1 &&
        drive != NRF_GPIO_PIN_D0S1 &&
        drive != NRF_GPIO_PIN_D0H1 &&
        drive != NRF_GPIO_PIN_S0D1 &&
        drive != NRF_GPIO_PIN_H0D1)
    {
        mp_raise_ValueError(MP_ERROR_TEXT("invalid drive mode"));
    }

    // Connect the GPIO input buffer if we've selected the mode as input
    nrf_gpio_pin_input_t input = (mode == NRF_GPIO_PIN_DIR_INPUT)
                                     ? NRF_GPIO_PIN_INPUT_CONNECT
                                     : NRF_GPIO_PIN_INPUT_DISCONNECT;

    // Set up the pin
    nrf_gpio_cfg(pin, mode, input, pull, drive, NRF_GPIO_PIN_NOSENSE);

    // Assign the pin to a new pin object
    machine_pin_obj_t *self = m_new_obj(machine_pin_obj_t);
    self->base.type = &machine_pin_type;
    self->pin = mp_obj_get_int(args[0].u_obj);

    // Return the new pin object
    return MP_OBJ_FROM_PTR(self);
}

/**
 * @brief Gets or sets the pin value.
 */
STATIC mp_obj_t machine_pin_call(mp_obj_t self_in, size_t n_args, size_t n_kw, const mp_obj_t *args)
{
    // Check the number of arguments given. No args is a read, 1 arg is a write
    mp_arg_check_num(n_args, n_kw, 0, 1, false);

    // Create a local object for accessing the pin number
    machine_pin_obj_t *self = self_in;

    // Figure out if pin is an input or an output
    nrf_gpio_pin_dir_t mode = nrf_gpio_pin_dir_get(self->pin);

    // If no args were give, read the pin value
    if (n_args == 0)
    {
        // If input, return the value on the pin
        if (mode == NRF_GPIO_PIN_DIR_INPUT)
        {
            return MP_OBJ_NEW_SMALL_INT(nrf_gpio_pin_read(self->pin));
        }

        // Otherwise read the output value that's currently set
        return MP_OBJ_NEW_SMALL_INT(nrf_gpio_pin_out_read(self->pin));
    }

    // Else if 1 arg was given, and pin is an input, return an error
    if (mode == NRF_GPIO_PIN_DIR_INPUT)
    {
        mp_raise_ValueError(MP_ERROR_TEXT("cannot set value of an input pin"));
    }

    // Otherwise if output, set the value
    mp_obj_is_true(args[0]) ? nrf_gpio_pin_set(self->pin)
                            : nrf_gpio_pin_clear(self->pin);

    return mp_const_none;
}

/**
 * @brief Method for setting up a pin for interrupts. Expects format as:
 *        myPin.irq(handler, trigger=irqEdgePolarity) where trigger is optional.
 */
STATIC mp_obj_t machine_pin_irq(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    // Create the allowed arguments table
    static const mp_arg_t allowed_args[] = {
        {MP_QSTR_handler, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_NONE}},
        {MP_QSTR_trigger, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_rom_obj = MP_ROM_INT(NRF_GPIOTE_POLARITY_TOGGLE)}},
    };

    // Parse args (remember the first arg is the pin ID, i.e [myPin].irq(...))
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // Get the pin my making a local object from the first argument
    machine_pin_obj_t *self = MP_OBJ_TO_PTR(pos_args[0]);

    // If pin is not an input, we can't set it to have an irq
    if (nrf_gpio_pin_dir_get(self->pin) == NRF_GPIO_PIN_DIR_OUTPUT)
    {
        mp_raise_ValueError(MP_ERROR_TEXT("cannot set irq for an output pin"));
    }

    // Set the interrupt trigger
    nrfx_gpiote_trigger_config_t trigger = {
        .trigger = mp_obj_get_int(args[1].u_obj),
        .p_in_channel = NULL,
    };

    // Apply the same pull direction as already configured
    nrfx_gpiote_input_config_t input = {
        .pull = nrf_gpio_pin_pull_get(self->pin),
    };

    // Set the handler function, and give the pin object as context
    nrfx_gpiote_handler_config_t handler = {
        .handler = pin_irq_handler,
        .p_context = self,
    };

    // Uninitialize the pin if it was already configured
    nrfx_gpiote_pin_uninit(self->pin);

    // Initialise the interrupt
    nrfx_gpiote_input_configure(self->pin, &input, &trigger, &handler);

    // Assign the handler pointer to the pin object
    self->irq_handler = args[0].u_obj;

    // Enable the interrupt event
    nrfx_gpiote_in_event_enable(self->pin, true);

    // Return nothing. The pin object is updated anyway
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(machine_pin_irq_obj, 1, machine_pin_irq);

/**
 * @brief Disables the IRQ for the given pin
 */
STATIC mp_obj_t machine_pin_irq_disable(mp_obj_t self_in)
{
    // Get the pin my making a local object from the first argument
    machine_pin_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // Disable the interrupt event
    nrfx_gpiote_in_event_disable(self->pin);

    // Return nothing
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_pin_irq_disable_obj, machine_pin_irq_disable);

/**
 * @brief Local class dictionary. Contains all the methods and constants of Pin.
 */
STATIC const mp_rom_map_elem_t machine_pin_locals_dict_table[] = {

    // Class methods
    {MP_ROM_QSTR(MP_QSTR_irq), MP_ROM_PTR(&machine_pin_irq_obj)},
    {MP_ROM_QSTR(MP_QSTR_irq_disable), MP_ROM_PTR(&machine_pin_irq_disable_obj)},

    // Pin modes
    {MP_ROM_QSTR(MP_QSTR_IN), MP_ROM_INT(NRF_GPIO_PIN_DIR_INPUT)},
    {MP_ROM_QSTR(MP_QSTR_OUT), MP_ROM_INT(NRF_GPIO_PIN_DIR_OUTPUT)},

    // Pin pull directions
    {MP_ROM_QSTR(MP_QSTR_PULL_UP), MP_ROM_INT(NRF_GPIO_PIN_PULLUP)},
    {MP_ROM_QSTR(MP_QSTR_PULL_DOWN), MP_ROM_INT(NRF_GPIO_PIN_PULLDOWN)},
    {MP_ROM_QSTR(MP_QSTR_PULL_DISABLED), MP_ROM_INT(NRF_GPIO_PIN_NOPULL)},

    // Pin drive modes
    {MP_ROM_QSTR(MP_QSTR_S0S1), MP_ROM_INT(NRF_GPIO_PIN_S0S1)},
    {MP_ROM_QSTR(MP_QSTR_H0S1), MP_ROM_INT(NRF_GPIO_PIN_H0S1)},
    {MP_ROM_QSTR(MP_QSTR_S0H1), MP_ROM_INT(NRF_GPIO_PIN_S0H1)},
    {MP_ROM_QSTR(MP_QSTR_H0H1), MP_ROM_INT(NRF_GPIO_PIN_H0H1)},
    {MP_ROM_QSTR(MP_QSTR_D0S1), MP_ROM_INT(NRF_GPIO_PIN_D0S1)},
    {MP_ROM_QSTR(MP_QSTR_D0H1), MP_ROM_INT(NRF_GPIO_PIN_D0H1)},
    {MP_ROM_QSTR(MP_QSTR_S0D1), MP_ROM_INT(NRF_GPIO_PIN_S0D1)},
    {MP_ROM_QSTR(MP_QSTR_H0D1), MP_ROM_INT(NRF_GPIO_PIN_H0D1)},

    // Pin IRQ directions
    {MP_ROM_QSTR(MP_QSTR_IRQ_RISING), MP_ROM_INT(NRF_GPIOTE_POLARITY_LOTOHI)},
    {MP_ROM_QSTR(MP_QSTR_IRQ_FALLING), MP_ROM_INT(NRF_GPIOTE_POLARITY_HITOLO)},
    {MP_ROM_QSTR(MP_QSTR_IRQ_TOGGLE), MP_ROM_INT(NRF_GPIOTE_POLARITY_TOGGLE)},

    // Both usuable Pin IDs
    {MP_ROM_QSTR(MP_QSTR_PIN_A1), MP_ROM_INT(4)},
    {MP_ROM_QSTR(MP_QSTR_PIN_A2), MP_ROM_INT(5)},
};
STATIC MP_DEFINE_CONST_DICT(machine_pin_locals_dict, machine_pin_locals_dict_table);

/**
 * @brief Class structure for the Pin object.
 */
const mp_obj_type_t machine_pin_type = {
    .base = {&mp_type_type},
    .name = MP_QSTR_Pin,
    .print = machine_pin_print,
    .make_new = machine_pin_make_new,
    .call = machine_pin_call,
    .locals_dict = (mp_obj_dict_t *)&machine_pin_locals_dict,
};