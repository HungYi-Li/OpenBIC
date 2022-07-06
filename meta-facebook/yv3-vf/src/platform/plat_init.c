#include <stdio.h>

#include "util_worker.h"

#include "plat_sensor_table.h"
#include "plat_class.h"
#include "plat_hwmon.h"
#include "plat_m2.h"
#include "plat_isr.h"
#include "plat_power_seq.h"
#include "plat_led.h"

void pal_pre_init()
{
	init_platform_config();
	init_e1s_config();
	init_worker(); // init util_worker
}

void pal_set_sys_status()
{
	pwr_related_pin_init();

	uint8_t i;
	for (i = M2_IDX_E_A; i < M2_IDX_E_MAX; i++) {
		pcie_sw_en_int_handler_m2x(i);
	}

	SSDLEDInit();

	// BIC up 1 sec handler
	K_WORK_DELAYABLE_DEFINE(up_1sec_handler, BICup1secTickHandler);
	k_work_schedule(&up_1sec_handler, K_SECONDS(1000));
	// BIC up 5 sec handler
	K_WORK_DELAYABLE_DEFINE(up_5sec_handler, BICup5secTickHandler);
	k_work_schedule(&up_5sec_handler, K_SECONDS(5000));
}
