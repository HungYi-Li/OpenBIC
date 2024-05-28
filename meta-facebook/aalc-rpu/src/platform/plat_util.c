/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdlib.h>
#include "plat_modbus.h"
#include "plat_util.h"
#include "modbus_server.h"
#include <logging/log.h>
#include "util_spi.h"
#include "libutil.h"

#define I2C_MASTER_READ_BACK_MAX_SIZE 16 // 16 registers

static uint16_t temp_read_length;
static uint16_t temp_read_data[I2C_MASTER_READ_BACK_MAX_SIZE];

LOG_MODULE_REGISTER(plat_util);

uint8_t modbus_command_i2c_master_write_read(modbus_command_mapping *cmd)
{
	CHECK_NULL_ARG_WITH_RETURN(cmd, MODBUS_EXC_ILLEGAL_DATA_VAL);

	// write data: bus(2Bytes), addr(2Bytes), read length(2Bytes), data(26Bytes)

	if (cmd->data_len <= 3) // check bus,addr,read length is not null
		return MODBUS_EXC_ILLEGAL_DATA_VAL;

	const uint8_t target_bus = cmd->data[0] & BIT_MASK(8); // get 7:0 bit data
	const uint8_t target_addr = cmd->data[1] & BIT_MASK(8);
	const uint8_t target_read_length = cmd->data[2] & BIT_MASK(8);
	I2C_MSG msg = { 0 };
	uint8_t retry = 5;
	msg.bus = target_bus;
	msg.target_addr = target_addr;
	msg.tx_len = cmd->data_len - 3; // write length need to -3 (bus,addr,read length)
	for (int i = 0; i < (cmd->data_len - 3); i++)
		msg.data[i] = cmd->data[i + 3] & BIT_MASK(8);

	if (target_read_length == 0) { // only write
		int result = i2c_master_write(&msg, retry);
		if (result != 0) {
			LOG_ERR("I2C write fail \n");
			return MODBUS_EXC_SERVER_DEVICE_FAILURE;
		}
		return MODBUS_EXC_NONE;
	}

	temp_read_length = target_read_length;
	msg.rx_len = (int)temp_read_length;
	int result = i2c_master_read(&msg, retry);
	if (result != 0) {
		LOG_ERR("I2C read fail \n");
		return MODBUS_EXC_SERVER_DEVICE_FAILURE;
	}

	memset(temp_read_data, 0xff, sizeof(temp_read_data));
	for (int i = 0; i < temp_read_length; i++)
		temp_read_data[i] = msg.data[i];

	return MODBUS_EXC_NONE;
}

uint8_t modbus_command_i2c_master_write_read_response(modbus_command_mapping *cmd)
{
	CHECK_NULL_ARG_WITH_RETURN(cmd, MODBUS_EXC_ILLEGAL_DATA_VAL);

	// write data: bus(2Bytes), addr(2Bytes), read length(2Bytes), data(reg:2Bytes+data:24Bytes)
	memcpy(cmd->data, temp_read_data, sizeof(uint16_t) * temp_read_length);

	return MODBUS_EXC_NONE;
}

void regs_reverse(uint16_t reg_len, uint16_t *data)
{
	CHECK_NULL_ARG(data);
	for (uint16_t i = 0; i < reg_len; i++)
		data[i] = sys_be16_to_cpu(data[i]);
}