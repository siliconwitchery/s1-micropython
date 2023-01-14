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

#ifndef NRFX_LOG_H__
#define NRFX_LOG_H__

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Macro for logging a message with the severity level ERROR.
 * @param format printf-style format string, optionally followed by arguments
 *               to be formatted and inserted in the resulting string.
 */
#define NRFX_LOG_ERROR(format, ...)

/**
 * @brief Macro for logging a message with the severity level WARNING.
 * @param format printf-style format string, optionally followed by arguments
 *               to be formatted and inserted in the resulting string.
 */
#define NRFX_LOG_WARNING(format, ...)

/**
 * @brief Macro for logging a message with the severity level INFO.
 * @param format printf-style format string, optionally followed by arguments
 *               to be formatted and inserted in the resulting string.
 */
#define NRFX_LOG_INFO(format, ...)

/**
 * @brief Macro for logging a message with the severity level DEBUG.
 * @param format printf-style format string, optionally followed by arguments
 *               to be formatted and inserted in the resulting string.
 */
#define NRFX_LOG_DEBUG(format, ...)

/**
 * @brief Macro for logging a memory dump with the severity level ERROR.
 * @param[in] p_memory Pointer to the memory region to be dumped.
 * @param[in] length   Length of the memory region in bytes.
 */
#define NRFX_LOG_HEXDUMP_ERROR(p_memory, length)

/**
 * @brief Macro for logging a memory dump with the severity level WARNING.
 * @param[in] p_memory Pointer to the memory region to be dumped.
 * @param[in] length   Length of the memory region in bytes.
 */
#define NRFX_LOG_HEXDUMP_WARNING(p_memory, length)

/**
 * @brief Macro for logging a memory dump with the severity level INFO.
 * @param[in] p_memory Pointer to the memory region to be dumped.
 * @param[in] length   Length of the memory region in bytes.
 */
#define NRFX_LOG_HEXDUMP_INFO(p_memory, length)

/**
 * @brief Macro for logging a memory dump with the severity level DEBUG.
 * @param[in] p_memory Pointer to the memory region to be dumped.
 * @param[in] length   Length of the memory region in bytes.
 */
#define NRFX_LOG_HEXDUMP_DEBUG(p_memory, length)

/**
 * @brief Macro for getting the textual representation of a given error code.
 * @param[in] error_code Error code.
 * @return String containing the textual representation of the error code.
 */
#define NRFX_LOG_ERROR_STRING_GET(error_code)

#ifdef __cplusplus
}
#endif

#endif // NRFX_LOG_H__