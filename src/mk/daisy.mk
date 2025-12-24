include ../mk/config.mk

# MCU / ABI
CPU_FLAGS := \
	-mcpu=cortex-m7 \
	-mthumb \
	-mfpu=fpv5-d16 \
	-mfloat-abi=hard

DEFS_COMMON := \
	-DUSE_HAL_DRIVER \
	-DSTM32H750xx \
	-DSTM32H750IB \
	-DCORE_CM7 \
	-DARM_MATH_CM7 \
	-DUSE_FULL_LL_DRIVER \
	-DHSE_VALUE=16000000

# Allow apps to add DEFS += ...
DEFS := $(DEFS_COMMON) $(DEFS)

# Includes (add your repo's shared headers too)
REPO_ROOT := ../../..

INCLUDES_COMMON := \
	-I$(REPO_ROOT)/src/common \
	-I$(LIBDAISY_DIR) \
	-I$(LIBDAISY_DIR)/src \
	-I$(LIBDAISY_DIR)/src/sys \
	-I$(LIBDAISY_DIR)/src/usbd \
	-I$(LIBDAISY_DIR)/src/usbh \
	-I$(LIBDAISY_DIR)/Drivers/CMSIS/Include \
	-I$(LIBDAISY_DIR)/Drivers/CMSIS/DSP/Include \
	-I$(LIBDAISY_DIR)/Drivers/CMSIS/Device/ST/STM32H7xx/Include \
	-I$(LIBDAISY_DIR)/Drivers/STM32H7xx_HAL_Driver/Inc \
	-I$(LIBDAISY_DIR)/Middlewares/ST/STM32_USB_Device_Library/Core/Inc \
	-I$(LIBDAISY_DIR)/Middlewares/ST/STM32_USB_Host_Library/Core/Inc \
	-I$(LIBDAISY_DIR)/Middlewares/ST/STM32_USB_Host_Library/Class/MSC/Inc \
	-I$(LIBDAISY_DIR)/Middlewares/Third_Party/FatFs/src \
	-I$(LIBDAISY_DIR)/core \
	-I$(DAISYSP_DIR)/Source

INCLUDES := $(INCLUDES_COMMON) $(INCLUDES)

CXXFLAGS := \
	$(CPU_FLAGS) \
	$(DEFS) \
	$(INCLUDES) \
	-O2 \
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

LDSCRIPT := $(LIBDAISY_DIR)/core/STM32H750IB_flash.lds

LDFLAGS := \
	$(CPU_FLAGS) \
	--specs=nano.specs \
	--specs=nosys.specs \
	-T$(LDSCRIPT) \
	-Wl,--gc-sections \
	-Wl,--print-memory-usage \
	-Wl,-Map=$(BUILD_DIR)/$(TARGET).map \
	-Wl,--defsym=__stack_size=0x10000

LIBS := \
	-L$(LIBDAISY_DIR)/build \
	-L$(DAISYSP_DIR)/build \
	-ldaisy \
	-ldaisysp \
	-lc \
	-lm \
	-lnosys

# Default source list if app doesn't specify
SRCS ?= main.cpp

OBJS := $(SRCS:%.cpp=$(BUILD_DIR)/%.o)

all: program-dfu

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: %.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJS)
	$(CXX) $^ $(LDFLAGS) $(LIBS) -o $@
	$(SIZE) $@

bin: $(BUILD_DIR)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $(BUILD_DIR)/$(TARGET).bin

program-dfu: bin
	$(DFU_UTIL) -a 0 -s $(DFU_ADDR):leave -D $(BUILD_DIR)/$(TARGET).bin -d $(DFU_ID) || true

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean bin program-dfu
