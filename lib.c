/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Damien P. George
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

#include <string.h>

// TODO Currently not used, but if we remove the GC later, these need proper
// implementation

// These memory functions are needed when the garbage collector is disabled.
// A full implementation should be provided, or the garbage collector enabled.
// The functions here are very simple.

extern char _heap_start;

void *malloc(size_t n)
{
    static char *cur_heap = NULL;
    if (cur_heap == NULL)
    {
        cur_heap = &_heap_start;
    }
    void *ptr = cur_heap;
    cur_heap += (n + 7) & ~7;
    return ptr;
}

void *realloc(void *ptr, size_t size)
{
    void *ptr2 = malloc(size);
    if (ptr && size)
    {
        memcpy(ptr2, ptr, size); // size may be greater than ptr's region, do copy anyway
    }
    return ptr2;
}

void free(void *p)
{
}