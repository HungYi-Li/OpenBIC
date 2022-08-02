#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include "hal_i2c.h"

#include "plat_m2.h"
#include "plat_sensor_table.h"
#include "plat_hwmon.h"

#define CEIL(A) (uint8_t)(((uint8_t)A == A) ? A : A + 1)

/* ina231 */
bool INA231_config_write(uint8_t i2c_bus, uint8_t slave_addr, uint8_t offset, uint8_t *buf,
			 uint8_t buf_len)
{
	if (!buf)
		return false;

	uint8_t retry = 5;
	I2C_MSG msg = { 0 };

	msg.bus = i2c_bus;
	msg.target_addr = slave_addr;
	msg.tx_len = buf_len + sizeof(offset);
	msg.data[0] = offset;
	memcpy(&msg.data[1], buf, buf_len);

	if (i2c_master_write(&msg, retry))
		return false;

	return true;
}

bool INA231_config_read(uint8_t i2c_bus, uint8_t slave_addr, uint8_t offset, uint8_t *buf,
			uint8_t buf_len)
{
	if (!buf)
		return false;

	uint8_t retry = 5;
	I2C_MSG msg;

	msg.bus = i2c_bus;
	msg.target_addr = slave_addr;
	msg.tx_len = sizeof(offset);
	msg.rx_len = buf_len;
	msg.data[0] = offset;

	if (i2c_master_read(&msg, retry)) {
		return false;
	}
	memcpy(buf, &msg.data[0], msg.rx_len);
	return true;
}

uint8_t ina231Request(uint8_t bus, uint8_t addr, uint8_t type, uint8_t *data)
{
	uint8_t retry = 3;

	while (retry != 0) {
		printf("%s() [%d] bus: %d, type: %x, retry: %d, \n", __func__, __LINE__, bus, type,
		       retry);
		if (INA231_config_write(bus, addr, type, data, 2) == true) {
			uint8_t buf[2] = { 0 };

			if (INA231_config_read(bus, addr, type, buf, 2) == true) {
				uint16_t read_data = buf[0] | buf[1] << 8;
				uint16_t write_data = data[0] | data[1] << 8;
				read_data &= write_data;
				if (!memcmp(&read_data, &write_data, sizeof(uint16_t)))
					return 1;
			}
		}

		retry--;
	}
	return 0;
}

uint8_t plat_ina231_init(uint8_t bus, uint8_t addr, uint8_t max_pwr)
{
	uint8_t type;
	uint8_t val[2];

	/* Set INA230 POL limit */
	/* reg0 = 0x1C, reg1 = 0x20 -> 0x1C20 (7200) -> 7200 * 25 / 1000 = 180 watt */
	type = 0x07;
	uint16_t tmp = max_pwr * 1000 / 25;
	val[0] = tmp >> 8;
	val[1] = tmp & 0xFF;
	if (!ina231Request(bus, addr, type, val))
		return 1;

	/* Enable POL flag */
	type = 0x06;
	val[0] = 0x08;
	val[1] = 0x08;
	if (!ina231Request(bus, addr, type, val))
		return 1;

	/* config calibration register for ina230 power reading */
	type = 0x05;

	val[0] = 0x02;
	val[1] = 0x00;

	if (!ina231Request(bus, addr, type, val))
		return 1;

	type = 0x00;
	val[0] = 0x4E;
	val[1] = 0x4F;
	if (!ina231Request(bus, addr, type, val))
		return 1;

	return 0;
}

void e1s_ina231_init(void)
{
	uint8_t i;
	for (i = M2_IDX_E_A; i < M2_IDX_E_MAX; i++) {
		if (plat_ina231_init(m2_idx2bus(i), I2C_ADDR_M2_INA231, 16)) {
			printf("!! config dev%d ina231 failed !!\n", i);
			return;
		}
	}
}

/* isl28022 */
bool ISL28022_config_write(uint8_t i2c_bus, uint8_t slave_addr, uint8_t offset, uint8_t *buf,
			   uint8_t buf_len)
{
	if (!buf)
		return false;

	uint8_t retry = 5;
	I2C_MSG msg = { 0 };

	msg.bus = i2c_bus;
	msg.target_addr = slave_addr;
	msg.tx_len = buf_len + sizeof(offset);
	msg.data[0] = offset;
	memcpy(&msg.data[1], buf, buf_len);

	if (i2c_master_write(&msg, retry))
		return false;

	return true;
}

bool ISL28022_config_read(uint8_t i2c_bus, uint8_t slave_addr, uint8_t offset, uint8_t *buf,
			  uint8_t buf_len)
{
	if (!buf)
		return false;

	uint8_t retry = 5;
	I2C_MSG msg;

	msg.bus = i2c_bus;
	msg.target_addr = slave_addr;
	msg.tx_len = sizeof(offset);
	msg.rx_len = buf_len;
	msg.data[0] = offset;

	if (i2c_master_read(&msg, retry)) {
		return false;
	}
	memcpy(buf, &msg.data[0], msg.rx_len);
	return true;
}

unsigned char isl28022Request(uint8_t i2c_bus, uint8_t type, uint8_t *data)
{
	uint8_t retry = 3;

	while (retry != 0) {
		if (ISL28022_config_write(i2c_bus, I2C_ADDR_M2_ISL28022, type, data, 2) == true) {
			uint8_t buf[2] = { 0 };
			if (ISL28022_config_read(i2c_bus, I2C_ADDR_M2_ISL28022, type, buf, 2) ==
			    true) {
				if (!memcmp(data, buf, 2))
					return 1;
			}
		}
		retry--;
	}
	return 0;
}
bool plat_isl28022_init(uint8_t dev_idx, uint8_t max_pwr)
{
	static int ina_status[M2_IDX_E_MAX][4] = { 0 };
	static uint8_t comp_status[M2_IDX_E_MAX] = { 0 };
	uint8_t i, type, reg_value[2], ret = 0;
	float set_pwr;

	for (i = 0; i < 4; i++) {
		if (ina_status[dev_idx][i] == 0) {
			switch (i) {
			case 0:
				type = 0x00;
				reg_value[0] = 0x7F;
				reg_value[1] = 0xFF;
				break;
			case 1: // Set INA230 POL limit, max_pwr / (0.00256/0.01) * 12
				// Set -128 Shunt Voltage Minimum to avoid alert
				set_pwr = max_pwr / ((0.00256 / 0.01) * 12);
				//SMC_DebugPrintf("*set_pwr = %d.%03d ceil = %x \n", (uint16_t)set_pwr, (uint16_t)(set_pwr * 1000)%1000 , (uint8_t)ceil(set_pwr));
				type = 0x06;
				reg_value[0] = CEIL(set_pwr);
				reg_value[1] = 0x80;
				break;
			case 2: //Enable POL flag
				type = 0x09;
				reg_value[0] = 0x00;
				reg_value[1] = 0x80;
				break;
			case 3: //config calibration register for ina230 power reading, 0.04096/((32/32768) * 0.01) = 4194
				type = 0x05;
				reg_value[0] = 0x10;
				reg_value[1] = 0x62;
				break;
			default:
				break;
			}
			ret = isl28022Request(m2_idx2bus(dev_idx), type, reg_value);
			ina_status[dev_idx][i] = (ret == 1) ? 1 : 0;
			comp_status[dev_idx] = (ina_status[dev_idx][i] == 1) ?
							     (comp_status[dev_idx] | (1 << i)) :
							     comp_status[dev_idx];
			//SMC_DebugPrintf("dev %d ina230-status %x,%x,%x,%x\n",dev_idx, ret,i, ina_status[dev_idx][i],comp_status[dev_idx]);
		}
	}
	if (comp_status[dev_idx] == 0x0f)
		return true;
	else
		return false;
}

void e1s_isl28022_init(void)
{
	uint8_t i;
	for (i = M2_IDX_E_A; i < M2_IDX_E_MAX; i++) {
		if (!plat_isl28022_init(i, 15)) //Set POL to 15 Watt
		{
			printf("!! config dev%d isl28022 failed !!\n", i);
			return;
		}
	}
}