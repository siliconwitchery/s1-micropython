# MicroPython Port – Silicon Witchery S1 Bluetooth & FPGA Module

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


## Building the binary

1. Clone this repo

    ```bash
    git clone <URL>.git
    ```

1. Build the `mpy-cross` tool

    ```bash
    cd micropython
    make -C mpy-cross
    ```

1. Ensure you have the `nrfx` lib downloaded using

    ```bash
    cd ports/s1-module/
    make submodules
    ```

1. Download the Bluetooth stack from Nordic

    ```bash
    make download-softdevice
    ```

1. Finally make the project

    ```bash
    make
    ```

1. To flash your device, use `make flash` to ensure the chip is erased, and Bluetooth stack is loaded along with the application. You will need a [J-Link compatible programmer](https://docs.siliconwitchery.com/s1-popout-board/s1-popout-board/#programming), and the [nRF command line tools](https://www.nordicsemi.com/Products/Development-tools/nrf-command-line-tools/download) installed

    ```bash
    make flash
    ```

## Learn more

For full details, be sure to check out the [documentation center](https://docs.siliconwitchery.com) 📚

Here you will find all the technical details for both hardware, and MicroPython usage.