# MicroPython on the S1 Bluetooth / FPGA Module

![S1 Module annotated](https://docs.siliconwitchery.com/images/annotated-module.png)

The S1 Module is based around the Nordic nRF52811, and Lattice iCE40 devices. This port is highly optimized for the small flash size of the nRF52811, and includes everything needed to interface with the FPGA.

The main interface to the Module is via the Bluetooth REPL. Use this [Web REPL](https://repl.siliconwitchery.com/) with Chrome to get started.

## Supported Features
- nRF52 peripherals:
    - Pin (All modes and drive strengths)
    - Pin interrupts & deep sleep wake
    - ADC (All modes)
    - RTC (Current time, and ms delay)
- FPGA interface
    - Run
    - Reset
    - Interrupt
    - SPI data transfer
- Integrated 32 Mbit flash
    - 256 byte page writes
    - 256 byte page reads
    - 4k block erase
    - Chip erase
- Integrated PMIC
    - Buck-boost voltage out setting 0.8V - 5.5V
    - FPGA IO voltage setting 0.8V - 3.45V
    - FPGA IO voltage load switch mode
    - FPGA main rail power down
    - Battery charging voltage and current settings


## Setting started

1. Set up the project:

    ```bash
    # Clone and jump into the directory
    git clone https://github.com/siliconwitchery/s1-micropython.git
    cd s1-micropython

    # Initialize the submodules
    git submodule update --init 

    # Build the mpy-cross tool
    make -C micropython/mpy-cross

    # Download the bluetooth stack from Nordic
    make download-softdevice
    ```

1. Ensure you have the latest [ARM GCC toolchain](https://developer.arm.com/downloads/-/gnu-rm) installed.

1. Build the project:

    ```bash
    make
    ```

1. To flash your device, check [this guide](https://docs.siliconwitchery.com/s1-popout-board/s1-popout-board/#programming). You will also need to download and install the [nRF command line tools](https://www.nordicsemi.com/Products/Development-tools/nrf-command-line-tools/download). To flash your S1, use the command:

    ```bash
    make flash
    ```

## Learn more

For full details, be sure to check out the [documentation center](https://docs.siliconwitchery.com) 📚

Here you will find all the technical details for both hardware, and MicroPython usage.