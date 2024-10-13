/*
 * Memory map for the STM32L47xx
 */

MEMORY {
    FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 1024K
    RAM (rwx)  : ORIGIN = 0x20000000, LENGTH = 96K
    RAM2 (rwx) : ORIGIN = 0x10000000, LENGTH = 32K
}
