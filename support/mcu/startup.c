// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/*
    Bare-metal start-up for QEMU's mps2 Cortex-M boards.
    Reset_Handler turns on the FPU and clears .bss ... sets up newlib with
    semihosting I/O, then calls main.
*/


#include <stdlib.h>

extern int main(void);
extern void __libc_init_array(void);
extern void initialise_monitor_handles(void);
extern char __bss_start__[], __bss_end__[], __stack[];

void _init(void){}
void _fini(void){}
static void hang(void){ for (;;){} }

void Reset_Handler(void){
	*(volatile unsigned long *)0xE000ED88 |= 0xFUL<<20; //! CPACR: enable the FPU (full access to CP10 and CP11)
	__asm volatile ("dsb\n isb");
	for (char *p=__bss_start__; p<__bss_end__; p++){ *p = 0; }
	initialise_monitor_handles();
	__libc_init_array();
	exit(main());
}

__attribute__((section(".vectors"), used)) static void(*const vectors[16])(void) = {
	(void (*)(void))(unsigned long) __stack, Reset_Handler, hang, hang, hang, hang, hang, 0, 0, 0, 0, hang, hang, 0, hang, hang
};
