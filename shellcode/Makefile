# SPDX-License-Identifier: GPL-2.0-only
# Copyright (c) 2026 feiyehua

CLANG ?= clang
PYTHON ?= python3
TARGET ?= armv7a-none-eabi
LINK_FLAT := link_flat.py

BINS := sep-pwn-smoke.bin sep-artk-derive.bin

.PHONY: all pwn-smoke artk clean
.SECONDARY:

all: $(BINS)

pwn-smoke: sep-pwn-smoke.bin
artk: sep-artk-derive.bin

%.o: %.S
	$(CLANG) --target=$(TARGET) -c $< -o $@

%.bin: %.o $(LINK_FLAT)
	$(PYTHON) $(LINK_FLAT) $< $@

clean:
	rm -f $(BINS) sep-pwn-smoke.o sep-artk-derive.o
