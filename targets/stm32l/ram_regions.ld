/*
 * Memory regions for the STM32L series
 *
 * RAM is reserved for the heap, rest goes to RAM2
 *
 * if you need to customize, create a custom version in your project root
 */

REGION_ALIAS("REGION_STACK", RAM2)
REGION_ALIAS("REGION_ISR", RAM2)
REGION_ALIAS("REGION_BSS", RAM2)
REGION_ALIAS("REGION_NOBSS", RAM2)
REGION_ALIAS("REGION_DATA", RAM2)
REGION_ALIAS("REGION_HEAP", RAM)
