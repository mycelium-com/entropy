/*
 * Memory space definitions for the bootloader.
 */

MEMORY
{
	rom (rx)  : ORIGIN = 0x00000000, LENGTH = 16K /* flash, boot region */
	ram (rwx) : ORIGIN = 0x20000000, LENGTH = 32K /* sram, 32K */
}
