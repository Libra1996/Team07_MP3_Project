// i2c lab
#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include "common_macros.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"

// TODO: Create i2c_slave_init.h
void i2c2__slave_init(uint8_t slave_address_to_respond_to);