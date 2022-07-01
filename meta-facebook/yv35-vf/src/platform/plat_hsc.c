#include <stdint.h>
#include <string.h>

#include "hal_i2c.h"

#include "plat_sensor_table.h"

#include "plat_hsc.h"

static uint8_t is_hsc_init = 0;

bool get_hsc_ready_flag(uint8_t sensor_number)
{
	return is_hsc_init ? true : false;
}

void set_hsc_ready_flag(uint8_t val)
{
	is_hsc_init = val;
}

uint8_t plat_adm1278_init(uint8_t bus, uint8_t addr) // ti: adm1278_init(uint8_t bus, uint8_t addr)
{
	int8_t reg = ADM1278_PMON_CONF_REG;
	uint8_t buf[2];
	uint8_t amd1278_retry = 3;

	while (amd1278_retry--) {
		uint8_t retry = 5;
		I2C_MSG msg;

		msg.bus = bus;
		msg.target_addr = addr;
		msg.tx_len = sizeof(reg);
		msg.rx_len = sizeof(buf);
		memcpy(&msg.data[0], &reg, sizeof(reg));

		if (i2c_master_read(&msg, retry)) {
			printf("HSC init failed!\n");
			continue;
		}

		buf[0] = msg.data[0];
		buf[1] = msg.data[1];
		buf[0] |= ADM1278_PMON_CONF_REG_TEMP1_EN;
		buf[1] |= ADM1278_PMON_CONF_REG_PWR_AVG;

		msg.data[0] = ADM1278_PMON_CONF_REG;
		msg.data[1] = buf[0];
		msg.data[2] = buf[1];
		msg.tx_len = 3;

		if (i2c_master_write(&msg, retry)) {
			continue;
		}
		break;
	}

	return (amd1278_retry) ? 0 : 1;
}