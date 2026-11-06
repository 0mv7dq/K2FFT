/*
	QEMU's mps3-an547 (Cortex-M55, hw/arm/mps2-tz.c), which boots secure from the ITCM at 0 w/ the SAU off, so every alias is secure.
 	Code goes in the ITCM ... constants, data and heap in SRAM ... the stack starts at the top of the DTCM
*/

MEMORY
{
	ITCM (rwx) : ORIGIN = 0x00000000, LENGTH = 512K
	SRAM (rwx) : ORIGIN = 0x01000000, LENGTH = 2M
	DTCM (rwx) : ORIGIN = 0x20000000, LENGTH = 512K
}
ENTRY(Reset_Handler)
SECTIONS
{
	.text : { KEEP(*(.vectors)) *(.text*) KEEP(*(.init)) KEEP(*(.fini)) } > ITCM
	.rodata : { *(.rodata*) } > SRAM
	.ARM.exidx : { *(.ARM.exidx*) } > SRAM
	.preinit_array : { __preinit_array_start = .; KEEP(*(.preinit_array)) __preinit_array_end = .; } > SRAM
	.init_array : { __init_array_start = .; KEEP(*(SORT(.init_array.*))) KEEP(*(.init_array)) __init_array_end = .; } > SRAM
	.fini_array : { __fini_array_start = .; KEEP(*(.fini_array)) __fini_array_end = .; } > SRAM
	.data : { *(.data*) } > SRAM
	.bss (NOLOAD) : { __bss_start__ = .; *(.bss*) *(COMMON) __bss_end__ = .; } > SRAM
	end = .;
	_end = .;
	__stack = ORIGIN(DTCM) + LENGTH(DTCM);
}
