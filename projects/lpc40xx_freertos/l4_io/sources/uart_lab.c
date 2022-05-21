#include "uart_lab.h"
#include "lpc40xx.h"

void set_dlab_bit(uart_number_e uart) {
  if (uart == UART_2) {
    LPC_UART2->LCR |= (1 << 7);
  } else if (uart == UART_3) {
    LPC_UART3->LCR |= (1 << 7);
  }
}

void clear_dlab_bit(uart_number_e uart) {
  if (uart == UART_2) {
    LPC_UART2->LCR &= ~(1 << 7);
  } else if (uart == UART_3) {
    LPC_UART3->LCR &= ~(1 << 7);
  }
}

void uart_lab__init(uart_number_e uart, uint32_t peripheral_clock, uint32_t baud_rate) {
  const uint32_t uart2_power = (1 << 24);
  const uint32_t uart3_power = (1 << 25);
  const uint16_t divider_16_bit = peripheral_clock * 1000 * 1000 / (16 * baud_rate);
  const uint8_t eight_bit_config = (3 << 0);
  if (uart == UART_2) {
    // Refer to LPC User manual and setup the register bits correctly
    // The first page of the UART chapter has good instructions
    // a) Power on Peripheral
    LPC_SC->PCONP |= uart2_power;
    // b) Setup DLL, DLM, FDR, LCR registers
    LPC_UART2->FCR |= (1 << 0); // FIFOEN
    LPC_UART2->LCR |= eight_bit_config;
    set_dlab_bit(UART_2);
    LPC_UART2->DLM |= (divider_16_bit >> 8) & 0xFF;
    LPC_UART2->DLL |= (divider_16_bit >> 0) & 0xFF;
    clear_dlab_bit(UART_2);
  } else if (uart == UART_3) {
    LPC_SC->PCONP |= uart3_power;
    LPC_UART3->FCR |= (1 << 0); // FIFOEN
    LPC_UART3->LCR |= eight_bit_config;
    set_dlab_bit(UART_3);
    LPC_UART3->DLM |= (divider_16_bit >> 8) & 0xFF;
    LPC_UART3->DLL |= (divider_16_bit >> 0) & 0xFF;
    clear_dlab_bit(UART_3);
  }
}

// Read the byte from RBR and actually save it to the pointer
bool uart_lab__polled_get(uart_number_e uart, char *input_byte) {
  const uint8_t rdr_bit = (1 << 0);
  if (uart == UART_2) {
    while (1) {
      // a) Check LSR for Receive Data Ready
      if (LPC_UART2->LSR & rdr_bit) {
        break;
      }
    }
    // b) Copy data from RBR register to input_byte
    *input_byte = LPC_UART2->RBR & 0xFF;
    return true;
  } else if (uart == UART_3) {
    while (1) {
      if (LPC_UART3->LSR & rdr_bit) {
        break;
      }
    }
    *input_byte = LPC_UART3->RBR & 0xFF;
    return true;
  }

  return false;
}

bool uart_lab__polled_put(uart_number_e uart, char output_byte) {
  const uint8_t thre_bit = (1 << 5);
  if (uart == UART_2) {
    while (1) {
      // a) Check LSR for Transmit Hold Register Empty
      if (LPC_UART2->LSR & thre_bit) {
        break;
      }
    }
    // b) Copy output_byte to THR register
    LPC_UART2->THR = output_byte;
    return true;
  } else if (uart == UART_3) {
    while (1) {
      // a) Check LSR for Transmit Hold Register Empty
      if (LPC_UART3->LSR & thre_bit) {
        break;
      }
    }
    // b) Copy output_byte to THR register
    LPC_UART3->THR = output_byte;
    return true;
  }

  return false;
}

// Part 2
// Private queue handle of our uart_lab.c
static QueueHandle_t your_uart_rx_queue;

// Private function of our uart_lab.c
static void your_receive_interrupt_uart2(void) {
  // TODO: Read the IIR register to figure out why you got interrupted
  if ((((LPC_UART2->IIR >> 1) & 0xF) == 0x2)) {
    // TODO: Based on IIR status, read the LSR register to confirm if there is data to be read
    while (!(LPC_UART2->LSR & (1 << 0))) {
      ;
    }
  }

  // TODO: Based on LSR status, read the RBR register and input the data to the RX Queue
  const char byte = LPC_UART2->RBR & 0xFF;
  xQueueSendFromISR(your_uart_rx_queue, &byte, NULL);
}

static void your_receive_interrupt_uart3(void) {
  if ((((LPC_UART3->IIR >> 1) & 0xF) == 0x2)) {
    while (1) {
      if (LPC_UART3->LSR & (1 << 0)) {
        break;
      }
    }
  }
  const char byte = LPC_UART3->RBR & 0xFF;
  xQueueSendFromISR(your_uart_rx_queue, &byte, NULL);
}

// Public function to enable UART interrupt
// TODO Declare this at the header file
void uart__enable_receive_interrupt(uart_number_e uart_number) {
  const uint8_t rcv_irs = (1 << 0);
  if (uart_number == UART_2) {
    // TODO: Use lpc_peripherals.h to attach your interrupt
    lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__UART2, your_receive_interrupt_uart2, "uart_2");

    // TODO: Enable UART receive interrupt by reading the LPC User manual
    // Hint: Read about the IER register
    clear_dlab_bit(UART_2);
    LPC_UART2->IER |= rcv_irs;
  } else if (uart_number == UART_3) {
    lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__UART3, your_receive_interrupt_uart3, "uart_3");
    clear_dlab_bit(UART_3);
    LPC_UART3->IER |= rcv_irs;
  }

  // TODO: Create your RX queue
  your_uart_rx_queue = xQueueCreate(1, sizeof(uint8_t));
}

// Public function to get a char from the queue (this function should work without modification)
// TODO: Declare this at the header file
bool uart_lab__get_char_from_queue(char *input_byte, uint32_t timeout) {
  return xQueueReceive(your_uart_rx_queue, input_byte, timeout);
}