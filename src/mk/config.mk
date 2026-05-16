MK_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
REPO_ROOT := $(abspath $(MK_DIR)/../..)
DAISY_DIR := $(REPO_ROOT)/External

LIBDAISY_DIR ?= $(DAISY_DIR)/libDaisy
DAISYSP_DIR ?= $(DAISY_DIR)/DaisySP
SYSTEM_FILES_DIR ?= $(LIBDAISY_DIR)/core

# Output folder *within each app dir*
BUILD_DIR ?= build

# Toolchain
# Default: Apple-style install from https://developer.arm.com/downloads/-/gnu-a
PREFERRED_GCC_PATH := /Applications/ArmGNUToolchain/14.3.rel1/arm-none-eabi/bin
ALT_APPLE_GCC_PATH := /Applications/ArmGNUToolchain/15.2.rel1/arm-none-eabi/bin
GCC_PATH ?= $(PREFERRED_GCC_PATH)

ifeq ($(wildcard $(GCC_PATH)/arm-none-eabi-g++),)
GCC_PATH := $(ALT_APPLE_GCC_PATH)
endif

ifeq ($(wildcard $(GCC_PATH)/arm-none-eabi-g++),)
ARM_CXX := $(shell command -v arm-none-eabi-g++ 2>/dev/null)
ifneq ($(ARM_CXX),)
GCC_PATH := $(patsubst %/,%,$(dir $(ARM_CXX)))
endif
endif

ifeq ($(wildcard $(GCC_PATH)/arm-none-eabi-g++),)
$(error No arm-none-eabi-g++ found. Install Arm GNU Toolchain under $(PREFERRED_GCC_PATH) or $(ALT_APPLE_GCC_PATH), or install arm-none-eabi-gcc on PATH (e.g. brew), or run: make GCC_PATH=/path/to/arm-none-eabi/bin)
endif

TOOLCHAIN_PREFIX ?= arm-none-eabi-
CC := $(GCC_PATH)/$(TOOLCHAIN_PREFIX)gcc
CXX := $(GCC_PATH)/$(TOOLCHAIN_PREFIX)g++
AR := $(GCC_PATH)/$(TOOLCHAIN_PREFIX)gcc-ar
RANLIB := $(GCC_PATH)/$(TOOLCHAIN_PREFIX)gcc-ranlib
OBJCOPY := $(GCC_PATH)/$(TOOLCHAIN_PREFIX)objcopy
SIZE := $(GCC_PATH)/$(TOOLCHAIN_PREFIX)size

# Build tuning
OPT_LEVEL ?= -Os
USE_LTO ?= 1

# Optional DaisySP base library
USE_DAISYSP ?= 0

# Optional DaisySP LGPL modules
USE_DAISYSP_LGPL ?= 0

# DFU / bootloader
DFU_UTIL ?= dfu-util
DFU_ID ?= ,0483:df11
DFU_ADDR ?= 0x08000000
BOOT_BIN ?= $(SYSTEM_FILES_DIR)/dsy_bootloader_v6_4-intdfu-2000ms.bin
APP_TYPE := BOOT_NONE
