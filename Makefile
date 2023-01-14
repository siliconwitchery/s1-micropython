# Include the core environment definitions
include micropython/py/mkenv.mk

# Include py core make definitions
include micropython/py/py.mk

# Set makefile-level MicroPython feature configurations
MICROPY_ROM_TEXT_COMPRESSION = 1

# Define toolchain and other tools
CROSS_COMPILE ?= arm-none-eabi-
DFU ?= micropython/tools/dfu.py
PYDFU ?= micropython/tools/pydfu.py

# Warning options
WARN = -Wall -Werror -Wdouble-promotion -Wfloat-conversion

# Build optimizations
OPT += -Os -fdata-sections -ffunction-sections 
OPT += -flto 
OPT += -fsingle-precision-constant
OPT += -fshort-enums
OPT += -fno-strict-aliasing
OPT += -fno-common
OPT += -g3

# Save some code space for performance-critical code
CSUPEROPT = -Os

# Add required build options
OPT += -std=gnu17
OPT += -mthumb
OPT += -mtune=cortex-m4
OPT += -mcpu=cortex-m4
OPT += -msoft-float
OPT += -mfloat-abi=soft
OPT += -mabi=aapcs

# Set defines
DEFS += -DNRF52811_XXAA
DEFS += -DNDEBUG

# Set linker options
LDFLAGS += -nostdlib
LDFLAGS += -Lnrfx/mdk -T nrf52811.ld
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -Xlinker -Map=$(@:.elf=.map)

# Add include paths
INC += -I.
INC += -Ibuild
INC += -Imicropython
INC += -Imicropython/lib/cmsis/inc
INC += -Imodules
INC += -Imodules/machine
INC += -Inrfx
INC += -Inrfx/drivers
INC += -Inrfx/drivers/include
INC += -Inrfx/drivers/src
INC += -Inrfx/hal
INC += -Inrfx/helpers
INC += -Inrfx/mdk
INC += -Isoftdevice/s112_nrf52_7.3.0_API/include
INC += -Isoftdevice/s112_nrf52_7.3.0_API/include/nrf52

# Assemble the C flags variable
CFLAGS += $(WARN) $(OPT) $(INC) $(DEFS)

# Define the required source files
SRC_C += main.c
SRC_C += modules/machine_adc.c
SRC_C += modules/machine_flash.c
SRC_C += modules/machine_fpga.c
SRC_C += modules/machine_pin.c
SRC_C += modules/machine_pmic.c
SRC_C += modules/machine_rtc.c
SRC_C += modules/modmachine.c
SRC_C += nrfx/drivers/src/nrfx_gpiote.c
SRC_C += nrfx/drivers/src/nrfx_rtc.c
SRC_C += nrfx/drivers/src/nrfx_saadc.c
SRC_C += nrfx/drivers/src/nrfx_spim.c
SRC_C += nrfx/drivers/src/nrfx_twim.c
SRC_C += nrfx/helpers/nrfx_flag32_allocator.c
SRC_C += nrfx/mdk/system_nrf52.c
SRC_C += shared/libc/printf.c
SRC_C += shared/libc/string0.c
SRC_C += shared/readline/readline.c
SRC_C += shared/runtime/pyexec.c
SRC_C += shared/runtime/stdout_helpers.c
SRC_C += startup_nrf52811.c

# List of sources for qstr extraction
SRC_QSTR += modules/machine_adc.c
SRC_QSTR += modules/machine_flash.c
SRC_QSTR += modules/machine_fpga.c
SRC_QSTR += modules/machine_pin.c
SRC_QSTR += modules/machine_pmic.c
SRC_QSTR += modules/machine_rtc.c
SRC_QSTR += modules/modmachine.c

# Define the required object files.
OBJ += $(PY_CORE_O)
OBJ += $(addprefix build/, $(SRC_C:.c=.o))

# Link required libraries
LIB += -lm -lc -lgcc

# Always make the binary and hex files
all: bin hex

bin: build/firmware.bin

build/firmware.bin: build/firmware.elf
	$(OBJCOPY) -O binary $< $@

hex: build/firmware.hex

build/firmware.hex: build/firmware.elf
	$(OBJCOPY) -O ihex $< $@

# Make the elf
build/firmware.elf: $(OBJ)
	$(ECHO) "LINK $@"
	$(Q)$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJ) $(LIB)
	$(Q)$(SIZE) $@

# Flashes both the MicroPython firmware and softdevice using nrfjprog
flash: build/firmware.hex
	nrfjprog --program softdevice/*.hex --chiperase -f nrf52
	nrfjprog --program $< -f nrf52
	nrfjprog --reset -f nrf52

# Downloads the softdevice files from Nordic
download-softdevice:
	mkdir -p softdevice
	curl -o softdevice/download.zip https://nsscprodmedia.blob.core.windows.net/prod/software-and-other-downloads/softdevices/s112/s112_nrf52_7.3.0.zip
	unzip -u softdevice/download.zip -d softdevice
	rm softdevice/download.zip

# Creates a release binary using mergehex
release: build/firmware.hex
	mergehex -q -m $< softdevice/*.hex -o build/micropython-s1-$(shell git describe).hex

# Include remaining core make rules.
include micropython/py/mkrules.mk