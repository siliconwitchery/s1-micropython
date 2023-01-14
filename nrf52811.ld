/*

  Linker script for use with micropython on the S1 module. RAM and Flash are
  configured for use with the S112 v7.2.0 softdevice with 2 custom UUIDs.

  Copyright (c) 2022 Raj Nakarja - Silicon Witchery AB

*/

/* Setting this value allows us to catch when the heap becomes to small */

min_reserved_heap = 4K;

/* Entry point for the application */

ENTRY(Reset_Handler)

/* Memory locations on the chip. Origin and size defined by the softdevice */

MEMORY
{
    FLASH (rx) : ORIGIN = 0x19000, LENGTH = 0x30000 - 0x19000
    RAM (rwx) :  ORIGIN = 0x20001d00, LENGTH = 0x6000 - 0x1d00
}

/* Complete layout of all the sections within memory */

SECTIONS
{
    /* Program code and other data goes into FLASH */

    .text :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector)) /* Startup code */
        *(.text)             /* .text sections (code) */
        *(.text*)            /* .text* sections (code) */
        *(.rodata)           /* .rodata sections (constants, strings, etc.) */
        *(.rodata*)          /* .rodata* sections (constants, strings, etc.) */
        
        . = ALIGN(4);
        _etext = .;          /* define a global symbol at end of code */
    } >FLASH

    /* This is used by the startup to initialize data */
    
    _sidata = LOADADDR(.data);

    /* This is the initialized data section. The program executes knowing that 
       the data is in the RAM but the loader puts the initial values in the 
       FLASH (inidata). It is one task of the startup to copy the initial values 
       from FLASH to RAM. */
    
    .data :
    {
        . = ALIGN(4);
        _sdata = .;        /* create a global symbol at data start; used by startup code in order to initialize the .data section in RAM */
        _ram_start = .;    /* create a global symbol at ram start for garbage collector */
        *(.data)           /* .data sections */
        *(.data*)          /* .data* sections */

        . = ALIGN(4);
        _edata = .;        /* define a global symbol at data end; used by startup code in order to initialize the .data section in RAM */
    } >RAM AT>FLASH

    /* Uninitialized data section */

    .bss :
    {
        . = ALIGN(4);
        _sbss = .;         /* define a global symbol at bss start; used by startup code */
        *(.bss)
        *(.bss*)
        *(COMMON)

        . = ALIGN(4);
        _ebss = .;         /* define a global symbol at bss end; used by startup code and GC */
    } >RAM

    .ARM.attributes 0 : 
    { 
        *(.ARM.attributes) 
    }
}

/* Define the end of the stack at the end of the RAM */

_stack_top = ORIGIN(RAM) + LENGTH(RAM);

/* Allow 2K ram for stack */

_stack_bot = _stack_top - 4K;

/* Heap goes from end of bss ram to the bottom of the stack */

_heap_start = _ebss;
_heap_end = _stack_bot;

/* Throw an error if the heap becomes too small */

ASSERT(_heap_end - _heap_start >= min_reserved_heap, "Heap has become too small")