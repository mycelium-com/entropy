/*
 * SRAM linker script for the firmware update helper applet.
 *
 * The author has waived all copyright and related or neighbouring rights
 * to this file and placed it in public domain.
 */

OUTPUT_FORMAT("elf32-littlearm", "elf32-littlearm", "elf32-littlearm")
OUTPUT_ARCH(arm)
SEARCH_DIR(.)

/*
 * Memory space definitions for 'rom' and 'ram' come from a processor-specific
 * file.
 */

ENTRY(applet)

/* Set __applet_size__ to fit it at the very top of RAM. */
__applet_size__ = 0x53c;
__param_size__  = 20;
__stack_size__  = 256;
__ram_end__ = ORIGIN(ram) + LENGTH(ram) - 4;

/* Section Definitions */
SECTIONS
{
    .text __ram_end__ - __param_size__ - __applet_size__ :
    {
	_stext = .;
	*(.entry)
        *(.text .text.* .gnu.linkonce.t.*)
        *(.glue_7t) *(.glue_7)
        *(.rodata .rodata* .gnu.linkonce.r.*)
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > ram

    .data ALIGN(4) :
    {
        *(.data .data.*)
        *(.bss .bss.*)
        *(COMMON)
    } > ram

    .bss __ram_end__ - __param_size__ (NOLOAD) :
    {
	*(.param)
	_ebss = .;
    } > ram

    ASSERT(_ebss == __ram_end__, "applet_param at unexpected address")

    __max_task__ = _stext - __stack_size__ - ORIGIN(ram);
}
