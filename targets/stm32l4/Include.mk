#
# Copyright (c) 2024 triaxis s.r.o.
# Licensed under the MIT license. See LICENSE.txt file in the repository root
# for full license information.
#
# stm32l4/Include.mk
#

TARGETS += stm32l cortex-m4f

STM32L4_MAKEFILE := $(call curmake)
STM32L4_DIR = $(dir $(STM32L4_MAKEFILE))

INCLUDE_DIRS += $(STM32L4_DIR)cmsis_device_l4/Include/
SOURCE_DIRS += $(STM32L4_DIR)cmsis_device_l4/Source/Templates/
