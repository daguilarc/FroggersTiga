include ../mk/config.mk

# MCU / ABI
CPU_FLAGS := \
	-mcpu=cortex-m7 \
	-mthumb \
	-mfpu=fpv5-d16 \
	-mfloat-abi=hard

FATFS_DIR := $(LIBDAISY_DIR)/Middlewares/Third_Party/FatFs/src
LIBDAISY_LIB := $(LIBDAISY_DIR)/build/libdaisy.a
DAISYSP_LIB := $(DAISYSP_DIR)/build/libdaisysp.a
DAISYSP_LGPL_LIB := $(DAISYSP_DIR)/DaisySP-LGPL/build/libdaisysp-lgpl.a
USE_DAISYSP_ANY := $(if $(filter 1,$(USE_DAISYSP) $(USE_DAISYSP_LGPL)),1,0)
ifeq ($(USE_LTO),1)
LTO_FLAGS := -flto
else
LTO_FLAGS :=
endif
VENDOR_OPT := $(OPT_LEVEL) $(LTO_FLAGS)

INTERNAL_ADDRESS := 0x08000000
QSPI_ADDRESS ?= 0x90040000
STM_PID := df11
DAISY_PID := df11

DEFS_COMMON := \
	-DUSE_HAL_DRIVER \
	-DSTM32H750xx \
	-DSTM32H750IB \
	-DCORE_CM7 \
	-DARM_MATH_CM7 \
	-DUSE_FULL_LL_DRIVER \
	-DHSE_VALUE=16000000 \
	-DFILEIO_ENABLE_FATFS_READER

INCLUDES_COMMON := \
	-I$(REPO_ROOT)/src/common \
	-I$(LIBDAISY_DIR) \
	-I$(LIBDAISY_DIR)/src \
	-I$(LIBDAISY_DIR)/src/sys \
	-I$(LIBDAISY_DIR)/src/usbd \
	-I$(LIBDAISY_DIR)/src/usbh \
	-I$(LIBDAISY_DIR)/Drivers/CMSIS_5/CMSIS/Core/Include \
	-I$(LIBDAISY_DIR)/Drivers/CMSIS-DSP/Include \
	-I$(LIBDAISY_DIR)/Drivers/CMSIS-Device/ST/STM32H7xx/Include \
	-I$(LIBDAISY_DIR)/Drivers/STM32H7xx_HAL_Driver/Inc \
	-I$(LIBDAISY_DIR)/Middlewares/ST/STM32_USB_Device_Library/Core/Inc \
	-I$(LIBDAISY_DIR)/Middlewares/ST/STM32_USB_Host_Library/Core/Inc \
	-I$(LIBDAISY_DIR)/Middlewares/ST/STM32_USB_Host_Library/Class/MSC/Inc \
	-I$(LIBDAISY_DIR)/Middlewares/ST/STM32_USB_Host_Library/Class/MIDI/Inc \
	-I$(FATFS_DIR) \
	-I$(SYSTEM_FILES_DIR)

LIBDIR := \
	-L$(LIBDAISY_DIR)/build

LIBS := \
	-ldaisy \
	-lc \
	-lm \
	-lnosys

BOOT_DEFS :=

ifeq ($(APP_TYPE),BOOT_NONE)
LDSCRIPT := $(SYSTEM_FILES_DIR)/STM32H750IB_flash.lds
USBPID := $(STM_PID)
DFU_ADDR := $(INTERNAL_ADDRESS)
else ifeq ($(APP_TYPE),BOOT_SRAM)
LDSCRIPT := $(SYSTEM_FILES_DIR)/STM32H750IB_sram.lds
USBPID := $(DAISY_PID)
DFU_ADDR := $(QSPI_ADDRESS)
BOOT_DEFS += -DBOOT_APP
else ifeq ($(APP_TYPE),BOOT_QSPI)
LDSCRIPT := $(SYSTEM_FILES_DIR)/STM32H750IB_qspi.lds
USBPID := $(DAISY_PID)
DFU_ADDR := $(QSPI_ADDRESS)
BOOT_DEFS += -DBOOT_APP
else
$(error Unknown APP_TYPE "$(APP_TYPE)")
endif

ifeq ($(USE_DAISYSP_ANY),1)
INCLUDES_COMMON += -I$(DAISYSP_DIR)/Source
LIBDIR += -L$(DAISYSP_DIR)/build
LIBS += -ldaisysp
endif

ifeq ($(USE_DAISYSP_LGPL),1)
INCLUDES_COMMON += -I$(DAISYSP_DIR)/DaisySP-LGPL/Source
LIBDIR += -L$(DAISYSP_DIR)/DaisySP-LGPL/build
LIBS += -ldaisysp-lgpl
endif

# Allow apps to add DEFS += ... and INCLUDES += ...
DEFS := $(DEFS_COMMON) $(BOOT_DEFS) $(DEFS)
INCLUDES := $(INCLUDES_COMMON) $(INCLUDES)

CXXFLAGS := \
	$(CPU_FLAGS) \
	$(DEFS) \
	$(INCLUDES) \
	$(OPT_LEVEL) \
	$(LTO_FLAGS) \
	-g \
	-ggdb \
	-std=gnu++17 \
	-fno-exceptions \
	-fno-rtti \
	-ffunction-sections \
	-fdata-sections \
	-Wall \
	-Wno-register \
	-Wno-missing-attributes \
	-Wno-stringop-overflow

LDFLAGS := \
	$(CPU_FLAGS) \
	--specs=nano.specs \
	--specs=nosys.specs \
	-T$(LDSCRIPT) \
	$(LTO_FLAGS) \
	$(LIBDIR) \
	$(LIBS) \
	-Wl,--gc-sections \
	-Wl,--print-memory-usage \
	-Wl,-Map=$(BUILD_DIR)/$(TARGET).map \
	-Wl,--defsym=__stack_size=0x10000

# Default source list if app doesn't specify
SRCS ?= main.cpp

OBJS := $(SRCS:%.cpp=$(BUILD_DIR)/%.o)
VENDORED_LIBS := $(LIBDAISY_LIB)

ifeq ($(USE_DAISYSP_ANY),1)
VENDORED_LIBS += $(DAISYSP_LIB)
endif

ifeq ($(USE_DAISYSP_LGPL),1)
VENDORED_LIBS += $(DAISYSP_LGPL_LIB)
endif

all: bin

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: %.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(LIBDAISY_LIB):
	$(MAKE) -C $(LIBDAISY_DIR) GCC_PATH="$(GCC_PATH)" OPT="$(VENDOR_OPT)" AR="$(AR)"

$(DAISYSP_LIB):
	$(MAKE) -C $(DAISYSP_DIR) GCC_PATH="$(GCC_PATH)" OPT="$(VENDOR_OPT)" AR="$(AR)"

$(DAISYSP_LGPL_LIB):
	$(MAKE) -C $(DAISYSP_DIR)/DaisySP-LGPL GCC_PATH="$(GCC_PATH)" OPT="$(VENDOR_OPT)" AR="$(AR)"

vendor-libs: $(VENDORED_LIBS)

$(BUILD_DIR)/$(TARGET).elf: $(OBJS) $(VENDORED_LIBS)
	$(CXX) $(OBJS) $(LDFLAGS) -o $@
	$(SIZE) $@

bin: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $(BUILD_DIR)/$(TARGET).bin

program-dfu: bin
	$(DFU_UTIL) -a 0 -s $(DFU_ADDR):leave -D $(BUILD_DIR)/$(TARGET).bin -d ,0483:$(USBPID)

program-boot:
	$(DFU_UTIL) -a 0 -s $(INTERNAL_ADDRESS):leave -D $(BOOT_BIN) -d ,0483:$(STM_PID)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean bin vendor-libs program-dfu program-boot
