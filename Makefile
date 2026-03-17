include src/mk/config.mk

LIBDAISY_DIR ?= External/libDaisy
DAISYSP_DIR ?= External/DaisySP
BOOT_BIN ?= $(LIBDAISY_DIR)/core/dsy_bootloader_v6_4-intdfu-2000ms.bin
DFU_UTIL ?= dfu-util
DFU_ID ?= ,0483:df11
INTERNAL_ADDRESS ?= 0x08000000

.PHONY: vendor-libs clean-vendor program-boot

vendor-libs:
	$(MAKE) -C $(LIBDAISY_DIR) GCC_PATH="$(GCC_PATH)" AR="$(AR)" OPT="$(OPT_LEVEL) $(if $(filter 1,$(USE_LTO)),-flto)"
ifneq ($(filter 1,$(USE_DAISYSP) $(USE_DAISYSP_LGPL)),)
vendor-libs:
	$(MAKE) -C $(DAISYSP_DIR) GCC_PATH="$(GCC_PATH)" AR="$(AR)" OPT="$(OPT_LEVEL) $(if $(filter 1,$(USE_LTO)),-flto)"
endif

clean-vendor:
	$(MAKE) -C $(LIBDAISY_DIR) clean GCC_PATH="$(GCC_PATH)"
ifneq ($(filter 1,$(USE_DAISYSP) $(USE_DAISYSP_LGPL)),)
clean-vendor:
	$(MAKE) -C $(DAISYSP_DIR) clean GCC_PATH="$(GCC_PATH)"
endif

program-boot:
	$(DFU_UTIL) -a 0 -s $(INTERNAL_ADDRESS):leave -D $(BOOT_BIN) -d $(DFU_ID)
