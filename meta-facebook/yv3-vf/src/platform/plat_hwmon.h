#include <stdio.h>
#include <stdint.h>

#define I2C_ADDR_MB_CPLD (0x1E >> 1)

void BICup1secTickHandler();
void BICup5secTickHandler();
void pcie_sw_en_int_handler_m2x(uint8_t idx);
int8_t mb_cpld_dev_prsnt_set(uint32_t idx, uint32_t val);