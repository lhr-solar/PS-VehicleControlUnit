# -------- Configuration --------

ARDUINO_CLI := arduino-cli
FQBN := esp32:esp32:esp32c6

# absolute directory containing this Makefile
ROOT := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))

FW_DIR := $(ROOT)
UART_BAUDRATE := 115200

.DEFAULT_GOAL := build

# -------- User Argument --------
# SKETCH=vcu  ->  esp32_vcu/esp32_vcu.ino

ifndef SKETCH
SKETCH_PATH :=
else
SKETCH_NAME := esp32_$(SKETCH)
SKETCH_PATH := $(FW_DIR)/$(SKETCH_NAME)/$(SKETCH_NAME).ino
endif

.PHONY: build
build:
ifndef SKETCH
	$(error Please provide SKETCH=<sketch_name>)
endif
	$(ARDUINO_CLI) compile --fqbn $(FQBN) $(SKETCH_PATH)

.PHONY: board_list
board_list:
	$(ARDUINO_CLI) board list

.PHONY: flash
flash:
ifndef SKETCH
	$(error Please provide SKETCH=<sketch_name>)
endif
ifndef PORT
	$(error Please provide PORT=<serial_port>)
endif
	$(ARDUINO_CLI) upload -p $(PORT) --fqbn $(FQBN) $(SKETCH_PATH)

.PHONY: serial
serial:
ifndef PORT
	$(error Please provide PORT=<serial_port>)
endif
	$(ARDUINO_CLI) monitor -p $(PORT) --config $(UART_BAUDRATE)

.PHONY: help
help:
	@echo ""
	@echo "Usage:"
	@echo "  make build SKETCH=<name>"
	@echo "  make flash SKETCH=<name> PORT=<serial>"
	@echo ""
	@echo "Example:"
	@echo "  make build SKETCH=vcu"
	@echo "  make flash SKETCH=vcu PORT=/dev/cu.usbmodem101"