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
#include "main.h"
#include "nrfx_gpiote.h"

/**
 * @brief Information about the configured done pin interrupt and handler.
 */
static struct
{
    bool enabled;
    mp_obj_t handler;
} done_pin_irq = {
    .enabled = false,
};

/**
 * @brief List of FPGA congifuration states.
 */
typedef enum
{
    FPGA_RUNNING,
    FPGA_CONFIGURING,
    FPGA_RESET,
} fpga_state_t;

/**
 * @brief The current FPGA congifuration state.
 */
static fpga_state_t fpga_state = FPGA_RESET;

/**
 * @brief IRQ handler for the done pin of the FPGA. Handles both configuration
 *        and user interrupts
 */
void fpga_done_pin_irq_handler(nrfx_gpiote_pin_t pin, nrfx_gpiote_trigger_t trigger, void *p_context)
{
    // If FPGA is in reset, we ignore any IRQ events
    if (fpga_state == FPGA_RESET)
    {
        return;
    }

    // If FPGA is configuring
    if (fpga_state == FPGA_CONFIGURING)
    {
        // Only care about rising edges here
        if (nrf_gpio_pin_read(16) == 1)
        {
            fpga_state = FPGA_RUNNING;
        }

        return;
    }

    // Otherwise, in running mode, and if an irq callback is enabled
    if (done_pin_irq.enabled)
    {
        // Call the handler, and pass the edge polarity to it
        mp_call_function_1(done_pin_irq.handler,
                           MP_OBJ_NEW_SMALL_INT(nrf_gpio_pin_read(16)));
    }
}

/**
 * @brief Initialises the FPGA module.
 */
void machine_fpga_init(void)
{
    // Configure the reset pin (nRF pin 20) to the FPGA
    nrf_gpio_cfg(20, NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT,
                 NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0S1, NRF_GPIO_PIN_NOSENSE);

    // Set the interrupt trigger for the done pin to capture both edges
    nrfx_gpiote_trigger_config_t trigger = {
        .trigger = NRFX_GPIOTE_TRIGGER_TOGGLE,
        .p_in_channel = NULL,
    };

    // Apply a pullup pull direction
    nrfx_gpiote_input_config_t input = {
        .pull = NRF_GPIO_PIN_PULLUP,
    };

    // Set the handler function
    nrfx_gpiote_handler_config_t handler = {
        .handler = fpga_done_pin_irq_handler,
        .p_context = NULL,
    };

    // Initialise the interrupt on nRF pin 16
    nrfx_gpiote_input_configure(16, &input, &trigger, &handler);

    // Enable the interrupt event for the pin
    nrfx_gpiote_in_event_enable(16, true);
}

/**
 * @brief Brings the FPGA out of reset and into the run state.
 */
STATIC mp_obj_t machine_fpga_run(void)
{
    // TODO do we need to wake up the flash first?

    // Set nRF pin 20 to bring the FPGA out of reset
    nrf_gpio_pin_set(20);

    // Mark the state as configuring
    fpga_state = FPGA_CONFIGURING;

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(machine_fpga_run_obj, machine_fpga_run);

/**
 * @brief Puts the FPGA into reset.
 */
STATIC mp_obj_t machine_fpga_reset(void)
{
    // Clear nRF pin 20 to put the FPGA into reset
    nrf_gpio_pin_clear(20);

    // Mark the state as reset
    fpga_state = FPGA_RESET;

    // TODO does keeping the FPGA done pin pulled high consume power?

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(machine_fpga_reset_obj, machine_fpga_reset);

/**
 * @brief Returns the reset and configuration status of the FPGA.
 */
STATIC mp_obj_t machine_fpga_status(void)
{
    switch (fpga_state)
    {
    case FPGA_RUNNING:
        return MP_OBJ_NEW_QSTR(MP_QSTR_FPGA_RUNNING);
        break;
    case FPGA_CONFIGURING:
        return MP_OBJ_NEW_QSTR(MP_QSTR_FPGA_CONFIGURING);
        break;
    case FPGA_RESET:
        return MP_OBJ_NEW_QSTR(MP_QSTR_FPGA_RESET);
        break;
    }

    // Shouldn't happen, but here to keep the warnings at bay
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(machine_fpga_status_obj, machine_fpga_status);

/**
 * @brief Method for setting up the done pin interrupt when in running mode.
 *        Expects format as FPGA.irq(handler).
 */
STATIC mp_obj_t machine_fpga_irq(mp_obj_t handler)
{
    // Set the callback function from the argument
    done_pin_irq.handler = handler;

    // Return
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_fpga_irq_obj, machine_fpga_irq);

/**
 * @brief Disables the interrupt for the done pin.
 */
STATIC mp_obj_t machine_fpga_irq_disable(void)
{
    // Disable the interrupt
    done_pin_irq.enabled = false;

    // Return
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(machine_fpga_irq_disable_obj, machine_fpga_irq_disable);

/**
 * @brief Reads n bytes from the FPGA, where n is the length of the read buffer.
 * @param read_obj: The read buffer as a bytearray() object
 */
STATIC mp_obj_t machine_fpga_read(mp_obj_t read_obj)
{
    // Create a read buffer from the object given
    mp_buffer_info_t read;
    mp_get_buffer_raise(read_obj, &read, MP_BUFFER_WRITE);

    // Receive the data
    spim_tx_rx(NULL, 0, (uint8_t *)&read.buf, read.len, FPGA);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_fpga_read_obj, machine_fpga_read);

/**
 * @brief Writes n bytes to the FPGA, where n is the length of the write buffer.
 * @param write_obj: The write buffer as a bytearray() object
 */
STATIC mp_obj_t machine_fpga_write(mp_obj_t write_obj)
{
    // Create a write buffer from the object given
    mp_buffer_info_t write;
    mp_get_buffer_raise(write_obj, &write, MP_BUFFER_READ);

    // Send the data
    spim_tx_rx((uint8_t *)&write.buf, write.len, NULL, 0, FPGA);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_fpga_write_obj, machine_fpga_write);

/**
 * @brief Simultaneously reads and writes n bytes from the FPGA, where n is the
 *        length of each buffer.
 * @param read_obj: The read buffer as a bytearray() object
 * @param write_obj: The write buffer as a bytearray() object
 */
STATIC mp_obj_t machine_fpga_read_write(mp_obj_t read_obj, mp_obj_t write_obj)
{
    // Create a read buffer from the object given
    mp_buffer_info_t read;
    mp_get_buffer_raise(read_obj, &read, MP_BUFFER_WRITE);

    // Create a write buffer from the object given
    mp_buffer_info_t write;
    mp_get_buffer_raise(write_obj, &write, MP_BUFFER_READ);

    // Send the data
    spim_tx_rx((uint8_t *)&write.buf, write.len, (uint8_t *)&read.buf, read.len, FPGA);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(machine_fpga_read_write_obj, machine_fpga_read_write);

/**
 * @brief Global module dictionary containing all of the methods and constants
 *        for the fpga module.
 */
STATIC const mp_rom_map_elem_t machine_fpga_locals_dict_table[] = {

    // Local methods
    {MP_ROM_QSTR(MP_QSTR_run), MP_ROM_PTR(&machine_fpga_run_obj)},
    {MP_ROM_QSTR(MP_QSTR_reset), MP_ROM_PTR(&machine_fpga_reset_obj)},
    {MP_ROM_QSTR(MP_QSTR_status), MP_ROM_PTR(&machine_fpga_status_obj)},
    {MP_ROM_QSTR(MP_QSTR_irq), MP_ROM_PTR(&machine_fpga_irq_obj)},
    {MP_ROM_QSTR(MP_QSTR_irq_disable), MP_ROM_PTR(&machine_fpga_irq_disable_obj)},
    {MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&machine_fpga_read_obj)},
    {MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&machine_fpga_write_obj)},
    {MP_ROM_QSTR(MP_QSTR_read_write), MP_ROM_PTR(&machine_fpga_read_write_obj)},
};
STATIC MP_DEFINE_CONST_DICT(machine_fpga_locals_dict, machine_fpga_locals_dict_table);

/**
 * @brief Module structure for the FPGA object.
 */
const mp_obj_type_t machine_fpga_type = {
    .base = {&mp_type_type},
    .name = MP_QSTR_FPGA,
    .print = NULL,
    .make_new = NULL,
    .call = NULL,
    .locals_dict = (mp_obj_dict_t *)&machine_fpga_locals_dict,
};