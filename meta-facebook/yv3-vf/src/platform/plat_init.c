#include <stdio.h>

#include "util_worker.h"

#include "plat_sensor_table.h"
#include "plat_class.h"
#include "plat_hwmon.h"
#include "plat_m2.h"
#include "plat_isr.h"
#include "plat_power_seq.h"
#include "plat_led.h"
#include "plat_gpio.h"
#include "ipmi.h"
uint32_t gpio_debounce_table[] = { FM_PRSNT_E1S_0_N, FM_PRSNT_E1S_1_N, FM_PRSNT_E1S_2_N,
				   FM_PRSNT_E1S_3_N };

SCU_CFG scu_cfg[] = {
	//register    value
	{ 0x7e6e2610, 0x00000100 },
};

void pal_pre_init()
{
	init_platform_config();
	init_e1s_config();
	init_worker(); // init util_worker
	set_gpio_debounce(gpio_debounce_table, ARRAY_SIZE(gpio_debounce_table), 10);
	scu_init(scu_cfg, ARRAY_SIZE(scu_cfg));
}

K_WORK_DELAYABLE_DEFINE(up_1sec_handler, BICup1secTickHandler);
K_WORK_DELAYABLE_DEFINE(up_5sec_handler, BICup5secTickHandler);

void pal_set_sys_status()
{
	pwr_related_pin_init();

	SSDLEDInit();

	// BIC up 1 sec handler
	k_work_schedule(&up_1sec_handler, K_SECONDS(1000));
	// BIC up 5 sec handler
	k_work_schedule(&up_5sec_handler, K_SECONDS(5000));
}

#define DEF_PROJ_GPIO_PRIORITY 78

DEVICE_DEFINE(PRE_DEF_PROJ_GPIO, "PRE_DEF_PROJ_GPIO_NAME", &gpio_init, NULL, NULL, NULL,
	      POST_KERNEL, DEF_PROJ_GPIO_PRIORITY, NULL);