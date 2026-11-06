/*
	QEMU's mps2-an505 (Cortex-M33), which starts secure at 0x10000000 w/ the SAU off, so the secure aliases, as Zephyr's an505 board.
 	Code, data and heap go in SSRAM1 ... the stack starts at the top of SSRAM2
*/

MEMORY
{
	SSRAM1 (rwx) : ORIGIN = 0x10000000, LENGTH = 4M
	SSRAM2 (rwx) : ORIGIN = 0x38000000, LENGTH = 2M
}
ENTRY(Reset_Handler)
SECTIONS
{
	.text : { KEEP(*(.vectors)) *(.text*) *(.rodata*) KEEP(*(.init)) KEEP(*(.fini)) } > SSRAM1
	.ARM.exidx : { *(.ARM.exidx*) } > SSRAM1
	.preinit_array : { __preinit_array_start = .; KEEP(*(.preinit_array)) __preinit_array_end = .; } > SSRAM1
	.init_array : { __init_array_start = .; KEEP(*(SORT(.init_array.*))) KEEP(*(.init_array)) __init_array_end = .; } > SSRAM1
	.fini_array : { __fini_array_start = .; KEEP(*(.fini_array)) __fini_array_end = .; } > SSRAM1
	.data : { *(.data*) } > SSRAM1
	.bss (NOLOAD) : { __bss_start__ = .; *(.bss*) *(COMMON) __bss_end__ = .; } > SSRAM1
	end = .;
	_end = .;
	__stack = ORIGIN(SSRAM2) + LENGTH(SSRAM2);
}
