#include <stdio.h>

#define I2C_ADDR_ADM1278 0x26

bool get_hsc_ready_flag(uint8_t sensor_number);
void set_hsc_ready_flag(uint8_t val);
uint8_t plat_adm1278_init(uint8_t bus, uint8_t addr);