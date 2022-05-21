#include "i2c_slave_init.h"

void i2c_slave_init(uint8_t slave_address_to_respond_to) {
  LPC_I2C2->ADR0 |= (slave_address_to_respond_to << 0); // set the address of slave to any number passed by user in main
  LPC_I2C2->CONSET = 0x44;                              // set I2EN and AA to 1 for slave mode
}