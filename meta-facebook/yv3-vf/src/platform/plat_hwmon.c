#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "plat_m2.h"
#include "plat_gpio.h"
#include "plat_power_seq.h"
#include "plat_class.h"
#include "plat_util.h"
#include "plat_i2c.h"
#include "plat_sensor_table.h"
#include "plat_tmp.h"

#include "plat_hwmon.h"

static void init_dev_prsnt_status(void)
{
	uint8_t i;
	for (i = M2_IDX_E_A; i < M2_IDX_E_MAX; i++) {
		if (!mb_cpld_dev_prsnt_set(i, m2_prsnt(i)))
			return;
	}
}

void BICup1secTickHandler()
{
	init_dev_prsnt_status();
}

// BICup5secTickHandler
void BICup5secTickHandler(void)
{
	/* For E1S re-spin ADC */
	if (get_e1s_adc_config() == CONFIG_ADC_INA231) {
		/* config on-board INA231 */
		e1s_ina231_init();
	} else if (get_e1s_adc_config() == CONFIG_ADC_ISL28022) {
		e1s_isl28022_init();
	}
}
void BICup5sec_work(struct k_work *work)
{
	if (!work)
		return;
	BICup5secTickHandler();
}
K_WORK_DEFINE(bic_up_5s, BICup5sec_work);
void BICup5sec_timer(struct k_timer *timer)
{
	if (!timer)
		return;
	k_work_submit(&bic_up_5s);
}
K_TIMER_DEFINE(bic_up_5s_timer, BICup5sec_timer, NULL);

void BICup5sec_handler(uint8_t ctrl) // 1: start, 0:stop
{
	if (ctrl)
		k_timer_start(&bic_up_5s_timer, K_SECONDS(5), K_SECONDS(5));
	else
		k_timer_stop(&bic_up_5s_timer);
}

int8_t mb_cpld_dev_prsnt_set(uint32_t idx, uint32_t val)
{
#define MB_CPLD_PRSNT_REG_1OU 0x05
	/*
 * device present offset
 * dev0, offset 4
 * dev1, offset 3
 * dev2, offset 2
 * dev3, offset 1
 */

	uint8_t reg = MB_CPLD_PRSNT_REG_1OU;
	uint8_t prsnt_ofs = idx + 1;
	uint8_t buf;

	uint8_t retry = 5;
	I2C_MSG msg;

	msg.bus = I2C_BUS_MB_CPLD;
	msg.target_addr = I2C_ADDR_MB_CPLD;
	msg.tx_len = sizeof(reg);
	msg.rx_len = sizeof(buf);
	memcpy(&msg.data[0], &reg, sizeof(reg));

	if (i2c_master_read(&msg, retry)) {
		printf("MB CPLD read failed!\n");
		return false;
	}

	buf = msg.data[0];
	buf = (buf & ~(1 << prsnt_ofs)) | (!val << prsnt_ofs);

	msg.tx_len = sizeof(buf) + 1;
	msg.data[0] = reg;
	memcpy(&msg.data[1], &buf, sizeof(buf));

	if (i2c_master_write(&msg, retry)) {
		return false;
	}

	return true;
}