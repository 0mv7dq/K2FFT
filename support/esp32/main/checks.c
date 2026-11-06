// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute


#include "esp_system.h"
#include <stdio.h>

int fft_main(void);
int spectrum_main(void);

void app_main(void){
	int failed = fft_main();
	failed |= spectrum_main();
	printf(failed ? "K2FFT checks FAILED\n" : "K2FFT checks complete\n");
	fflush(stdout);
	esp_restart();
}
