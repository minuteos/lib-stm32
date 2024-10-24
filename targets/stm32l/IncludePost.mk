#
# Copyright (c) 2024 triaxis s.r.o.
# Licensed under the MIT license. See LICENSE.txt file in the repository root
# for full license information.
#
# stm32l/IncludePost.mk
#

# The STM32L series have quite slow flashes, with many wait states at higher
# frequencies - loading literals from flash causes unnecesssary frequent
# cache misses, this option generates slightly larger code with the literals
# loaded using MOVW/MOVT instead of LDR x, [PC, ...]
ARCH_FLAGS += -mslow-flash-data
