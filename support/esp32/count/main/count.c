// Copyright (c) 2011-2026 0MV7DQ @ The Antisonic Research Institute


#include "esp_system.h"
#include <stdio.h>

int count_main(void);

void app_main(void){
	count_main();
	fflush(stdout);
	esp_restart(); /* QEMU's -no-reboot ends here */
}
