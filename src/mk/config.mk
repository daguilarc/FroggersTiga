# Where DaisyExamples lives (your "convention")
DAISY_DIR := $(HOME)/Desktop/DaisyExamples

LIBDAISY_DIR := $(DAISY_DIR)/libDaisy
DAISYSP_DIR  := $(DAISY_DIR)/DaisySP

# Output folder *within each app dir*
BUILD_DIR ?= build

# Toolchain (explicit paths)
CC  := /opt/homebrew/bin/arm-none-eabi-gcc-14.3.1
CXX := /Applications/ArmGNUToolchain/14.3.rel1/arm-none-eabi/bin/arm-none-eabi-g++
AR  := /opt/homebrew/bin/arm-none-eabi-ar
OBJCOPY := /opt/homebrew/bin/arm-none-eabi-objcopy
SIZE := /opt/homebrew/bin/arm-none-eabi-size

# DFU
DFU_UTIL ?= dfu-util
DFU_ID   ?= ,0483:df11
DFU_ADDR ?= 0x08000000
