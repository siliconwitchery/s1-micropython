# Include the core environment definitions; this will set $(TOP).
include ../../py/mkenv.mk

# Include py core make definitions.
include $(TOP)/py/py.mk

# Ensure we have the nrfx submodule
GIT_SUBMODULES = lib/nrfx

# Set makefile-level MicroPython feature configurations
MICROPY_ROM_TEXT_COMPRESSION = 1

# Define toolchain and other tools.
CROSS_COMPILE ?= arm-none-eabi-
DFU ?= $(TOP)/tools/dfu.py
PYDFU ?= $(TOP)/tools/pydfu.py

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
LDFLAGS += -L$(TOP)/lib/nrfx/mdk -T nrf52811.ld
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -Xlinker -Map=$(@:.elf=.map)

# Add include paths
INC += -I.
INC += -I$(TOP)
INC += -I$(BUILD)
INC += -I../../lib/cmsis/inc
INC += -I../../lib/nrfx
INC += -I../../lib/nrfx/drivers
INC += -I../../lib/nrfx/drivers/include
INC += -I../../lib/nrfx/drivers/src
INC += -I../../lib/nrfx/hal
INC += -I../../lib/nrfx/helpers
INC += -I../../lib/nrfx/mdk
INC += -Imodules
INC += -Imodules/machine
INC += -Isoftdevice/s112_nrf52_7.2.0_API/include
INC += -Isoftdevice/s112_nrf52_7.2.0_API/include/nrf52

# Assemble the C flags variable
CFLAGS += $(WARN) $(OPT) $(INC) $(DEFS)

# Define the required source files
SRC_C += main.c
# SRC_C += lib.c # TODO malloc if we remove GC later
SRC_C += startup_nrf52811.c
SRC_C += lib/nrfx/drivers/src/nrfx_saadc.c
SRC_C += lib/nrfx/drivers/src/nrfx_spim.c
SRC_C += lib/nrfx/drivers/src/nrfx_gpiote.c
SRC_C += lib/nrfx/drivers/src/nrfx_rtc.c
SRC_C += lib/nrfx/drivers/src/nrfx_twim.c
SRC_C += lib/nrfx/helpers/nrfx_flag32_allocator.c
SRC_C += lib/nrfx/mdk/system_nrf52.c
SRC_C += modules/modmachine.c
SRC_C += modules/machine_adc.c
SRC_C += modules/machine_flash.c
SRC_C += modules/machine_fpga.c
SRC_C += modules/machine_pmic.c
SRC_C += modules/machine_pin.c
SRC_C += modules/machine_rtc.c
SRC_C += shared/libc/printf.c
SRC_C += shared/libc/string0.c
SRC_C += shared/readline/readline.c
SRC_C += shared/runtime/pyexec.c
SRC_C += shared/runtime/stdout_helpers.c

# List of sources for qstr extraction
SRC_QSTR += modules/modmachine.c
SRC_QSTR += modules/machine_adc.c
SRC_QSTR += modules/machine_flash.c
SRC_QSTR += modules/machine_fpga.c
SRC_QSTR += modules/machine_pmic.c
SRC_QSTR += modules/machine_pin.c
SRC_QSTR += modules/machine_rtc.c

# Define the required object files.
OBJ += $(PY_CORE_O)
OBJ += $(addprefix $(BUILD)/, $(SRC_C:.c=.o))

# Link required libraries
LIB += -lm -lc -lgcc

# Always make the binary and hex files
all: bin hex

bin: $(BUILD)/firmware.bin

$(BUILD)/firmware.bin: $(BUILD)/firmware.elf
	$(OBJCOPY) -O binary $< $@

hex: $(BUILD)/firmware.hex

$(BUILD)/firmware.hex: $(BUILD)/firmware.elf
	$(OBJCOPY) -O ihex $< $@

# Make the elf
$(BUILD)/firmware.elf: $(OBJ)
	$(ECHO) "LINK $@"
	$(Q)$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJ) $(LIB)
	$(Q)$(SIZE) $@

# Flashes both the MicroPython firmware and softdevice using nrfjprog
flash: $(BUILD)/firmware.hex
	nrfjprog --program softdevice/*.hex --chiperase -f nrf52
	nrfjprog --program $< -f nrf52
	nrfjprog --reset -f nrf52

# Downloads the softdevice files from Nordic
download-softdevice:
	mkdir -p softdevice
	wget https://www.nordicsemi.com/-/media/Software-and-other-downloads\
	/SoftDevices/S112/s112nrf52720.zip -O softdevice/download.zip
	unzip -u softdevice/download.zip -d softdevice
	rm softdevice/download.zip

# Creates a release binary using mergehex
release: $(BUILD)/firmware.hex
	mergehex -q -m $< softdevice/*.hex -o build/micropython-s1-$(shell git describe).hex

# Include remaining core make rules.
include $(TOP)/py/mkrules.mk