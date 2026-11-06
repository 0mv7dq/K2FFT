// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute

/* checks.c's app_main, for tools/count-all.c */
#include "esp_chip_info.h"
#include "esp_cpu.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include <stdio.h>

int count_main(void);

/* count-all.c's BENCH_TICKS on a real chip: CCOUNT */
unsigned long bench_ticks(void){
	return (unsigned long)esp_cpu_get_cycle_count();
}

void app_main(void){
#ifdef BENCH_WAIT
	while (getchar()==EOF){ /* a real board's serial reader attaches after the flash's reset */
		clearerr(stdin);
		vTaskDelay(pdMS_TO_TICKS(10));
	}
#endif
	esp_chip_info_t chip;
	esp_chip_info(&chip);
	printf("chip %s rev %d, %d MHz\n", CONFIG_IDF_TARGET, chip.revision, CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ);
	count_main();
	printf("count done\n");
	fflush(stdout);
	esp_restart(); /* QEMU's -no-reboot ends here */
}
