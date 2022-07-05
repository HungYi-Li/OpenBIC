#include <zephyr.h>
#include <stdio.h>
#include <stdbool.h>
#include "ipmi.h"
#include "ipmb.h"
#include "hal_gpio.h"
#include "plat_m2.h"
#include "plat_class.h"
#include "plat_util.h"
#include "plat_isr.h"
#include "plat_i2c.h"
#include "plat_hwmon.h"
#include "plat_hsc.h"

#include "plat_power_seq.h"

static uint8_t is_pwrgd_p12v_aux_100ms;
static uint8_t sensor_pwrgd_1s[M2_IDX_E_MAX];

void pwr_related_pin_init(void)
{
	uint8_t i;

	/* init FM_P3V3_E1S_X_SW_EN */
	for (i = M2_IDX_E_A; i < M2_IDX_E_MAX; i++)
		fm_p3v3_sw_en(i, m2_prsnt(i) & gpio_get(FM_AUX_PWR_EN));

	/* init FM_P12V_EDGE_EN */
	gpio_set(FM_P12V_EDGE_EN, gpio_get(FM_POWER_EN));

	/* Set the default value for FM_P12V_E1S_X_EN, FM_CLKBUF_EN, CLKBUF_E1S_X_OE_N and RST_BIC_E1S_X_N */
	pwrgd_p12v_aux_int_handler();
}

uint8_t get_dev_pwrgd(uint8_t idx)
{
	return sensor_pwrgd_1s[idx];
}

static void set_sensor_pwrgd_1s(uint8_t idx)
{
	sensor_pwrgd_1s[idx] = 1;
}

uint8_t fm_p3v3_sw_en(uint8_t idx, uint8_t val)
{
	const uint8_t pin = (idx == M2_IDX_E_A) ? FM_P3V3_E1S_0_SW_EN :
			    (idx == M2_IDX_E_B) ? FM_P3V3_E1S_1_SW_EN :
			    (idx == M2_IDX_E_C) ? FM_P3V3_E1S_2_SW_EN :
			    (idx == M2_IDX_E_D) ? FM_P3V3_E1S_3_SW_EN :
							0xFF;

	if (pin == 0xFF)
		return 0;
	//If value is want to set to enable(1) set GPIO direction to input (0).
	//Else if input pin want to set to disable(0), set GPIO value to 0 and then set GPIO direction to output (1).
	if (val) {
		gpio_conf(pin, 0);
	} else {
		gpio_set(pin, val);
		gpio_conf(pin, 1);
	}

	return 1;
}

uint8_t fm_p12v_sw_en(uint8_t idx, uint8_t val)
{
	const uint8_t pin = (idx == M2_IDX_E_A) ? FM_P12V_E1S_0_EN :
			    (idx == M2_IDX_E_B) ? FM_P12V_E1S_1_EN :
			    (idx == M2_IDX_E_C) ? FM_P12V_E1S_2_EN :
			    (idx == M2_IDX_E_D) ? FM_P12V_E1S_3_EN :
							0xFF;

	if (pin == 0xFF)
		return 0;

	if (get_e1s_hsc_config() != CONFIG_HSC_BYPASS)
		gpio_set(pin, val);
	else {
		//If value is want to set to enable(1) set GPIO direction to input (0).
		//Else if input pin want to set to disable(0), set GPIO value to 0 and then set GPIO direction to output (1).
		if (val) {
			gpio_conf(pin, 0);
		} else {
			gpio_set(pin, val);
			gpio_conf(pin, 1);
		}

		dev_pwrgd_handler(idx);
	}
	return 1;
}

uint8_t get_fm_p12v_sw_en(uint8_t idx)
{
	const uint8_t pin = (idx == M2_IDX_E_A) ? FM_P12V_E1S_0_EN :
			    (idx == M2_IDX_E_B) ? FM_P12V_E1S_1_EN :
			    (idx == M2_IDX_E_C) ? FM_P12V_E1S_2_EN :
			    (idx == M2_IDX_E_D) ? FM_P12V_E1S_3_EN :
							0xFF;

	if (pin == 0xFF)
		return 0;

	if (get_e1s_hsc_config() == CONFIG_HSC_BYPASS)
		return get_gpio_conf(pin) ? gpio_get(pin) : 1;
	else
		return gpio_get(pin);
}

uint8_t clkbuf_oe_en(uint8_t idx, uint8_t val)
{
	const uint8_t pin = (idx == M2_IDX_E_A) ? CLKBUF_E1S_0_OE_N :
			    (idx == M2_IDX_E_B) ? CLKBUF_E1S_1_OE_N :
			    (idx == M2_IDX_E_C) ? CLKBUF_E1S_2_OE_N :
			    (idx == M2_IDX_E_D) ? CLKBUF_E1S_3_OE_N :
							0xFF;

	if (pin == 0xFF)
		return 0;

	gpio_set(pin, !val);
	return 1;
}

uint8_t get_fm_pwrdis_status(uint8_t idx)
{
	const uint8_t pin = (idx == M2_IDX_E_A) ? FM_PWRDIS_E1S_0 :
			    (idx == M2_IDX_E_B) ? FM_PWRDIS_E1S_1 :
			    (idx == M2_IDX_E_C) ? FM_PWRDIS_E1S_2 :
			    (idx == M2_IDX_E_D) ? FM_PWRDIS_E1S_3 :
							0xFF;

	if (pin == 0xFF)
		return 0;

	return gpio_get(pin);
}
uint8_t fm_pwrdis_en(uint8_t idx, uint8_t val)
{
	const uint8_t pin = (idx == M2_IDX_E_A) ? FM_PWRDIS_E1S_0 :
			    (idx == M2_IDX_E_B) ? FM_PWRDIS_E1S_1 :
			    (idx == M2_IDX_E_C) ? FM_PWRDIS_E1S_2 :
			    (idx == M2_IDX_E_D) ? FM_PWRDIS_E1S_3 :
							0xFF;

	if (pin == 0xFF)
		return 0;

	gpio_set(pin, val);
	return 1;
}

void check_dc_off_process(void)
{
	uint8_t i;

	for (i = 0; i < M2_IDX_E_MAX; i++) {
		if (get_fm_p12v_sw_en(i))
			break;
	}

	if (i == M2_IDX_E_MAX) {
		dev_12v_fault_hander(); // control PWRGD_EXP_PWROK & LED_PWRGD_P12V_E1S_ALL
		gpio_set(FM_P12V_EDGE_EN, 0);
		gpio_set(FM_CLKBUF_EN, 0);
	}
}

/*
 * +--------+---------------+------------------------------------------+
 * |        | AC            | ON                                       |
 * |        +---------------+----------------+-------------------------+
 * |        | DC            | OFF            | ON                      |
 * |        +---------------+-------+--------+-------+--------+--------+
 * |        | PRSNT         | ON    | OFF    | ON    | ON     | OFF    |
 * |        +---------------+-------+--------+-------+--------+--------+
 * |        | DRIVE PWR     | OFF            | ON    | OFF    | OFF    |
 * +--------+---------------+-------+--------+-------+--------+--------+
 * | FM_P12V_E1S_X_EN       | L     | L      | H     | L      | L      |
 * +------------------------+-------+--------+-------+--------+--------+
 * | FM_P3V3_E1S_X_SW_EN    | H     | L      | H     | H      | L      |
 * +------------------------+-------+--------+-------+--------+--------+
 * | CLKBUF_E1S_X_OE_N      | H     | H      | L     | L      | H      |
 * +------------------------+-------+--------+-------+--------+--------+
 */

uint8_t m2_dev_power_switch_with_pwrdis_chk(uint8_t idx, uint8_t enable, uint8_t chk_pwrdis,
					    uint8_t force_ctl_3v3)
{
	static uint8_t is_force_disable_3v3[M2_IDX_E_MAX];

	const uint8_t dc_en = gpio_get(FM_POWER_EN);
	const uint8_t hsc_pwrgd = get_e1s_pwrgd();
	const uint8_t prsnt = m2_prsnt(idx);

	const uint8_t en_12v = dc_en && hsc_pwrgd && prsnt && enable;
	const uint8_t en_3v3 = force_ctl_3v3 ? enable : prsnt;
	// const uint8_t en_clk = dc_en && hsc_pwrgd && prsnt;
	const uint8_t en_clk = enable && prsnt;

	//   SMC_DebugPrintf("[%s] idx = %d , enable = %d , chk_pwrdis = %d , force_ctl_3v3 = %d \n",__func__ , idx , enable,chk_pwrdis ,force_ctl_3v3 );
	//   SMC_DebugPrintf("[%s] en_12v = %d , en_3v3 = %d,  en_clk = %d \n", __func__ , en_12v, en_3v3 , en_clk);
	//   SMC_DebugPrintf("\n");

	/* if the PWRDIS is enable by user, don't control the drive power by 12V/3V3 SW. */
	if (chk_pwrdis) {
		if (get_fm_pwrdis_status(idx))
			return 0;
	}

	/* for the drive power off, the 12v should be disable before 3v3 */
	if (!enable) {
		clkbuf_oe_en(idx, en_clk);
		delay_function(3, fm_p12v_sw_en, idx, en_12v);
	}

	if (force_ctl_3v3 && (enable == 0)) {
		is_force_disable_3v3[idx] = 1;
		fm_p3v3_sw_en(idx, en_3v3);
	} else if (force_ctl_3v3 && (enable == 1)) {
		is_force_disable_3v3[idx] = 0;
	}

	// check whether force disable
	if (!is_force_disable_3v3[idx])
		fm_p3v3_sw_en(idx, en_3v3);

	if (enable) {
		fm_p12v_sw_en(idx, en_12v);

		//delay 40 ms to enable clkbuf after enable 12V
		delay_function(50, clkbuf_oe_en, idx, en_clk);
	}

	if (!dc_en) {
		delay_function(20, check_dc_off_process, 0, 0);
	}

	return 1;
}

uint8_t m2_dev_power_switch(uint8_t idx, uint8_t enable)
{
	return m2_dev_power_switch_with_pwrdis_chk(idx, enable, 1, 0);
}

uint8_t device_all_power_set(uint8_t idx, uint8_t set_val)
{
	uint8_t is_on = (set_val & DEV_PWR_ON ? 1 : 0);
	uint8_t chk_pwrdis = (set_val & DEV_CHK_DISABLE ? 1 : 0);
	uint8_t force_ctl_3v3 = (set_val & DEV_FORCE_3V3 ? 1 : 0);

	//    SMC_DebugPrintf("[%s] -- idx = %d , is_on = %d , chk_pwrdis = %d , force_ctl_3v3 = %d \n" , __func__ , idx , is_on , chk_pwrdis , force_ctl_3v3);
	//    SMC_DebugPrintf("set_val = 0x%x \n" , set_val);
	if (is_on) {
		if (set_val & DEV_PWR_CTRL)
			m2_dev_power_switch_with_pwrdis_chk(idx, is_on, chk_pwrdis, force_ctl_3v3);
		if (set_val & DEV_PWRDIS_EN) {
			fm_pwrdis_en(idx, !is_on);
			delay_function(40, clkbuf_oe_en, idx, is_on);
			delay_function(120, rst_edsff, idx, is_on);
		}
		if (set_val & DEV_PCIE_RST)
			delay_function(120, rst_edsff, idx, is_on);
		if (set_val & DEV_PRSNT_SET)
			delay_function(140, mb_cpld_dev_prsnt_set, idx, is_on);
		return 0;
	}

	if (set_val & DEV_PRSNT_SET)
		mb_cpld_dev_prsnt_set(idx, is_on);

	if (set_val & DEV_PCIE_RST)
		rst_edsff(idx, is_on);

	if (set_val & DEV_PWRDIS_EN) {
		fm_pwrdis_en(idx, !is_on);
		delay_function(4, rst_edsff, idx, is_on);
		delay_function(8, clkbuf_oe_en, idx, is_on);
	}

	if (set_val & DEV_PWR_CTRL) {
		if (force_ctl_3v3)
			m2_dev_power_switch_with_pwrdis_chk(idx, is_on, chk_pwrdis, force_ctl_3v3);
		else
			delay_function(3, m2_dev_power_switch, idx, is_on);
	}

	return 0;
}

void dev_pwrgd_handler(uint8_t idx)
{
	const uint8_t pin = (idx == M2_IDX_E_A) ? FM_P12V_E1S_0_EN :
			    (idx == M2_IDX_E_B) ? FM_P12V_E1S_1_EN :
			    (idx == M2_IDX_E_C) ? FM_P12V_E1S_2_EN :
			    (idx == M2_IDX_E_D) ? FM_P12V_E1S_3_EN :
							0xFF;
	if (pin == 0xFF)
		return;

	if (get_fm_p12v_sw_en(idx)) {
		delay_function(1000, set_sensor_pwrgd_1s, idx, 0);
	} else {
		sensor_pwrgd_1s[idx] = 0;
	}
}

void pwrgd_p12v_aux_100ms_set(uint32_t val, uint32_t unused1)
{
	if (val) {
		/*Only ADM1278 has i2c and need to initial in all HSC configuration*/

		if (get_e1s_hsc_config() == CONFIG_HSC_ADM1278) {
			if (plat_adm1278_init(I2C_BUS_ADM1278, I2C_ADDR_ADM1278))
				set_hsc_ready_flag(1);
		} else {
			set_hsc_ready_flag(1);
		}
	} else {
		set_hsc_ready_flag(0);
	}

	is_pwrgd_p12v_aux_100ms = val;
	dev_rst();
}

uint8_t pwrgd_p12v_aux_100ms_get(void)
{
	return is_pwrgd_p12v_aux_100ms;
}

#define DEV_PWRGD_HANDLER(idx)                                                                     \
	void dev_pwrgd_handler_dev##idx(void)                                                      \
	{                                                                                          \
		dev_pwrgd_handler(idx);                                                            \
	}

DEV_PWRGD_HANDLER(0);
DEV_PWRGD_HANDLER(1);
DEV_PWRGD_HANDLER(2);
DEV_PWRGD_HANDLER(3);