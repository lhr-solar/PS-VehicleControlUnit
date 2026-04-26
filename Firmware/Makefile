# ----- VCU Makefile -----

# crazy colors ------------------
RED=\033[0;31m
GREEN=\033[0;32m
ORANGE=\033[0;33m
BLUE=\033[0;34m
PURPLE=\033[0;35m
CYAN=\033[0;36m
LIGHTGRAY=\033[0;37m
DARKGRAY=\033[1;30m
YELLOW=\033[0;33m
NC=\033[0m # No Color
#---------------------------------


# Project Configuration ----------
TEST ?= main
PROJECT_TARGET ?= stm32g473xx
VCU_BOOTLOADER_BRINGUP ?= false
VCU_BOOTLOADER_BRINGUP_LEVEL ?= 0
VCU_BOOTLOADER_FULL_DIAG ?= false
ifeq ($(VCU_BOOTLOADER_BRINGUP),true)
ifneq ($(FIRMWARE_TYPE),app)
$(error VCU_BOOTLOADER_BRINGUP=true must be built with FIRMWARE_TYPE=app)
endif
endif
ifeq ($(VCU_BOOTLOADER_FULL_DIAG),true)
ifneq ($(FIRMWARE_TYPE),app)
$(error VCU_BOOTLOADER_FULL_DIAG=true must be built with FIRMWARE_TYPE=app)
endif
endif
# STM32G473RET6 

# source and include directories
PROJECT_C_INCLUDES = $(wildcard */Inc)
PROJECT_C_SOURCES = $(wildcard */Src/*.c)

# build directories
PROJECT_BUILD_DIR = build
BUILD_MAKEFILE_DIR = Embedded-Sharepoint

# generate paths
MAKEFILE_DIR = $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

PROJECT_MAIN_DIR ?= core/Src/main

ifneq ($(TEST), main)
PROJECT_C_SOURCES := $(filter-out $(PROJECT_MAIN_DIR).c, $(PROJECT_C_SOURCES))
PROJECT_C_SOURCES := $(addprefix $(MAKEFILE_DIR)/, $(PROJECT_C_SOURCES) tests/$(TEST)_test.c)
else
PROJECT_C_SOURCES := $(addprefix $(MAKEFILE_DIR)/, $(PROJECT_C_SOURCES))
endif

# debug
ifeq ($(PRINT_DEBUG), true)
$(info SOURCES: $(PROJECT_C_SOURCES))
$(info INCLUDES: $(PROJECT_C_INCLUDES))
endif

# generate paths (cont.d)
PROJECT_C_INCLUDES := $(addprefix $(MAKEFILE_DIR)/, $(PROJECT_C_INCLUDES))
PROJECT_BUILD_DIR := $(addprefix $(MAKEFILE_DIR)/, $(PROJECT_BUILD_DIR))

# Additional defines
PROJECT_MACROS = USART3_RX_QUEUE_SIZE=100

ifeq ($(VCU_BOOTLOADER_BRINGUP),true)
PROJECT_MACROS += VCU_BOOTLOADER_BRINGUP
PROJECT_MACROS += VCU_BOOTLOADER_BRINGUP_LEVEL=$(VCU_BOOTLOADER_BRINGUP_LEVEL)
endif

ifeq ($(VCU_BOOTLOADER_FULL_DIAG),true)
PROJECT_MACROS += VCU_BOOTLOADER_FULL_DIAG
endif

# export variables 
export PROJECT_TARGET
export PROJECT_C_SOURCES
export PROJECT_C_INCLUDES
export PROJECT_BUILD_DIR
export PROJECT_MACROS


#-------------------------------
# Clang
BEAR_PREFIX := 
# check if bear is installed
BEAR_INSTALLED := $(shell command -v bear >/dev/null 2>&1 && echo yes || echo no)

# define path of .vscode (create if missing)
VS_CODE_DIR := $(MAKEFILE_DIR)/.vscode
$(shell mkdir -p $(VS_CODE_DIR))

ifeq ($(BEAR_INSTALLED),yes)
BEAR_PREFIX := bear --output $(VS_CODE_DIR)/compile_commands.json --append --
endif


# Build --------------------------
ifeq ($(MAKECMDGOALS),)
default: build_code
else ifeq ($(MAKECMDGOALS), all)
all: build_code
else
%:
	$(BEAR_PREFIX) $(MAKE) -C $(BUILD_MAKEFILE_DIR) $(MAKECMDGOALS)
endif


build_code:
ifneq ($(TEST), main)
	@echo -e "Making ${PURPLE}$(PROJECT_TARGET)${NC}build for ${BLUE}TEST=${PURPLE} ${TEST}${NC}"
else
	@echo -e "Making ${PURPLE}$(PROJECT_TARGET)${NC}build with ${ORANGE}no test.${NC}"
endif
	$(BEAR_PREFIX) $(MAKE) -C $(BUILD_MAKEFILE_DIR) all -j
	@echo -e "${BLUE}Compiled for VCU! Splendid! Jolly Good!!${NC}"
#---------------------------------

#-------------------------------
# Flash
flash: 	
	$(MAKE) -C $(BUILD_MAKEFILE_DIR) flash


#-------------------------------
# ESP32 Build
.PHONY: esp32
ESP32_DIR := $(MAKEFILE_DIR)/esp32_fw

esp32:
ifndef SKETCH
	$(error Please provide SKETCH=<name>)
endif
	$(MAKE) -C $(ESP32_DIR) SKETCH=$(SKETCH)
#-------------------------------

#-------------------------------
# ESP32 flash
.PHONY: esp32_flash
esp32_flash:
ifndef SKETCH
	$(error Please provide SKETCH=<name>)
endif
ifndef PORT
	$(error Please provide PORT=<serial_port>)
endif
	$(MAKE) -C $(ESP32_DIR) flash SKETCH=$(SKETCH) PORT=$(PORT)
#-------------------------------

# Help
.PHONY: help
help:
	@echo -e "Format: ${ORANGE}make  ${GREEN}PROJECT_TARGET=${PURPLE}<stm-target>${NC} ${BLUE}TEST=${PURPLE}<Test name>${NC}"
	@echo "- Simply running '${ORANGE}make${NC}' or '${ORANGE}make${NC} all' will compile the production code."
	@echo "- Specify ${GREEN}TARGET${NC}if different than default; ${BLUE}TEST${NC}is optional. \n"
	@echo "- Use ${GREEN}FIRMWARE_TYPE=${PURPLE}app${NC} to link after the bootloader."
	@echo "- Use ${GREEN}VCU_BOOTLOADER_BRINGUP=${PURPLE}true${NC} for isolated bootloader bring-up firmware.\n"
	@echo "- Use ${GREEN}VCU_BOOTLOADER_FULL_DIAG=${PURPLE}true${NC} to keep the normal app path and light init/RTOS checkpoints.\n"
	@echo "- Use ${GREEN}VCU_BOOTLOADER_BRINGUP_LEVEL=${PURPLE}<0..5>${NC} to add bring-up features one at a time:"
	@echo "  0: raw GPIO blink only, before HAL_Init"
	@echo "  1: add HAL_Init, then raw GPIO blink"
	@echo "  2: add HAL_Delay"
	@echo "  3: add SystemClock_Config"
	@echo "  4: add UART bootloader command polling"
	@echo "  5: add CAN init and heartbeat frames\n"

	@echo "${GREEN}PROJECT_TARGET${NC}( default is ${PURPLE}stm32g473xx${NC}):"
	@echo "- Specify stm32 target by entering the full stm chip number.\n"

	@echo "${BLUE}TEST${NC}( OPTIONAL ):"
	@echo "- If you want to run a test, specify ${BLUE}TEST=${PURPLE}<Test name>${NC}, with ${PURPLE}<Test name>${NC}"
	@echo "   being the exact name of the test file ${RED} without${NC} the .c suffix and test_ suffix. \n"

#-------------- 
# Documentation
# .PHONY: docs
# docs:
# 	cd $(BUILD_MAKEFILE_DIR) && mkdocs serve