# BMS High-Voltage Contactor Overcurrent Monitor — SIL & Static Verification Makefile
CC      ?= gcc
CFLAGS  := -std=c99 -O2 -Wall -Wextra -Werror -Wpedantic -Wconversion -Wshadow -Wundef -fno-common -Iinclude

BUILD_DIR := build
ECU_OBJ   := $(BUILD_DIR)/bms_fault_monitor.o
SIL_BIN   := $(BUILD_DIR)/sil_runner

.PHONY: all build test misra footprint arm-cross check clean

all: check

build:
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c src/bms_fault_monitor.c -o $(ECU_OBJ)
	$(CC) $(CFLAGS) $(ECU_OBJ) tests/sil_harness.c -o $(SIL_BIN)

test: build
	./$(SIL_BIN)

misra:
	python3 scripts/misra_check.py

footprint: build
	python3 scripts/footprint_check.py

check: misra footprint test

clean:
	rm -rf $(BUILD_DIR)
