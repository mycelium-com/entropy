/*
 * Memory space definitions for SAM4L2.
 * First 16 kB of flash are reserved for SAM-BA.
 */

MEMORY
{
	rom (rx)  : ORIGIN = 0x00004000, LENGTH = 112K /* flash, 128K */
	ram (rwx) : ORIGIN = 0x20000000, LENGTH =  32K /* sram, 32K */
}
