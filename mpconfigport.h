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

#include <stdint.h>
#include <alloca.h>

// Board configuration
#define MICROPY_HW_BOARD_NAME "s1 module"
#define MICROPY_HW_MCU_NAME "nrf52811"

// Use the minimal starting configuration
#define MICROPY_CONFIG_ROM_LEVEL (MICROPY_CONFIG_ROM_LEVEL_MINIMUM)

// Enable the complier so we have REPL access
#define MICROPY_ENABLE_COMPILER (1)

// Enable float support, but don't need complex numbers
#define MICROPY_FLOAT_IMPL (MICROPY_FLOAT_IMPL_FLOAT)
#define MICROPY_PY_BUILTINS_COMPLEX (0)

// Enable byte arrays
#define MICROPY_PY_BUILTINS_BYTEARRAY (1)

////////////////////////////////////////////////////////////////////////////////
// TODO These are nice to have features. If space is needed, we can reduce them
////////////////////////////////////////////////////////////////////////////////

// Use normal error reporting
#define MICROPY_ERROR_REPORTING (MICROPY_ERROR_REPORTING_NORMAL)

// Enable garbage collector module. Takes around 2kB of flash
// TODO How is the garbage collector used?
#define MICROPY_ENABLE_GC (1)

// Enable tab completion and auto indenting TODO tab completion seems broken
#define MICROPY_HELPER_REPL (1)
#define MICROPY_REPL_AUTO_INDENT (1)

// Enable the help() function, help text and help('modules')
#define MICROPY_PY_BUILTINS_HELP (0) // TODO Re-enable this once we make space
#define MICROPY_PY_BUILTINS_HELP_TEXT help_text
#define MICROPY_PY_BUILTINS_HELP_MODULES (1)

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// Type definitions for the specific machine
#define MICROPY_MAKE_POINTER_CALLABLE(p) ((void *)((mp_uint_t)(p) | 1))

#define MP_SSIZE_MAX (0x7fffffff)

typedef int mp_int_t;           // must be pointer size
typedef unsigned int mp_uint_t; // must be pointer size
typedef long mp_off_t;

// Alias to port specific root pointers
#define MP_STATE_PORT MP_STATE_VM

// Root pointer for REPL history
#define MICROPY_PORT_ROOT_POINTERS const char *readline_hist[8];