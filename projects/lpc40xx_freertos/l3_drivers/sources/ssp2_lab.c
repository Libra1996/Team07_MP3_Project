#include <stdbool.h>
#include <stddef.h>

#include "ssp2_lab.h"

#include "clock.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"
#include <stdio.h>

void ssp2__init(uint32_t max_clock_mhz) {
  // Refer to LPC User manual and setup the register bits correctly
  // a) Power on Peripheral
  uint32_t ssp2_power_bit = (1 << 20);
  LPC_SC->PCONP |= ssp2_power_bit;
  // b) Setup control registers CR0 and CR1
  // uint32_t scr_bit = 0;
  LPC_SSP2->CR0 |= (0b111 << 0);
  uint32_t ssp2_enable = (1 << 1);
  LPC_SSP2->CR1 |= ssp2_enable;
  // c) Setup prescalar register to be <= max_clock_mhz
  uint8_t divider = 2;
  const uint32_t cpu_clock_mhz = clock__get_core_clock_hz() / 1000000UL;

  while (max_clock_mhz < (cpu_clock_mhz / divider) && divider <= 254) {
    divider += 2;
  }
  // fprintf(stderr, "divider: %d \n", divider);

  LPC_SSP2->CPSR = divider;
}

uint8_t ssp2__exchange_byte_lab(uint8_t data_out) {
  // Configure the Data register(DR) to send and receive data by checking the SPI peripheral status register
  LPC_SSP2->DR = data_out;

  const uint32_t busy_bit = (1 << 4);
  while (1) {
    if (LPC_SSP2->SR & busy_bit) {
      ; // do nothing and wait
    } else {
      break;
    }
  }

  return LPC_SSP2->DR & 0xFF;
}