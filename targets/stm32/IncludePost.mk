#
# Copyright (c) 2024 triaxis s.r.o.
# Licensed under the MIT license. See LICENSE.txt file in the repository root
# for full license information.
#
# stm32/Include.mk
#

ifeq (1,$(STM32_GEN_DFU))

PRIMARY_DFU = $(OUTPUT).dfu
# use EXE suffix because it's required by windows and won't hurt others :)
DFUTOOL = $(OBJDIR)elf2dfuse.exe
DFUSRC = $(dir $(call curmake))../../elf2dfuse/elf2dfuse.c

$(DFUTOOL): $(DFUSRC)
	$(HOST_CC) $< -o $@

$(PRIMARY_DFU): $(PRIMARY_OUTPUT) $(DFUTOOL)
	$(DFUTOOL) $(PRIMARY_OUTPUT) $@

main: $(PRIMARY_DFU)

endif
