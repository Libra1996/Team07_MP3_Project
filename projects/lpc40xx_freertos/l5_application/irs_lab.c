// @file irs_lab.c
#include "irs_lab.h"
#include "lpc40xx.h"
#include <stdio.h>

int logic_that_you_will_write(uint32_t);
void clear_pin_interrupt(uint32_t, uint32_t);

// Note: You may want another separate array for falling vs. rising edge callbacks
static function_pointer_t gpio0_callbacks_R[32];
static function_pointer_t gpio0_callbacks_F[32];
// static function_pointer_t gpio2_callbacks_R[32];
// static function_pointer_t gpio2_callbacks_F[32];

void gpio0__attach_interrupt(uint32_t pin, gpio_interrupt_e interrupt_type, function_pointer_t callback) {
  // 1) Store the callback based on the pin at gpio0_callbacks
  // 2) Configure GPIO 0 pin for rising or falling edge
  if (interrupt_type == GPIO_INTR__RISING_EDGE) {
    gpio0_callbacks_R[pin] = callback;
    LPC_GPIOINT->IO0IntEnR |= (1 << pin);
  } else {
    gpio0_callbacks_F[pin] = callback;
    LPC_GPIOINT->IO0IntEnF |= (1 << pin);
  }
}

// We wrote some of the implementation for you
void gpio0__interrupt_dispatcher(void) {
  // Check which pin generated the interrupt
  int pin_that_generated_interrupt = logic_that_you_will_write(0);
  function_pointer_t attached_user_handler;
  if (pin_that_generated_interrupt > 32) {
    pin_that_generated_interrupt = pin_that_generated_interrupt - 32;
    attached_user_handler = gpio0_callbacks_R[pin_that_generated_interrupt];
  } else {
    attached_user_handler = gpio0_callbacks_F[pin_that_generated_interrupt];
  }

  // fprintf(stderr, "pin: %d", pin_that_generated_interrupt);

  // Invoke the user registered callback, and then clear the interrupt
  attached_user_handler();
  clear_pin_interrupt(0, pin_that_generated_interrupt);
}

int logic_that_you_will_write(uint32_t port_num) {
  for (int i = 0; i < 32; i++) {
    if (port_num == 0) {
      if (LPC_GPIOINT->IO0IntStatF & (1 << i)) {
        return i;
      } else if (LPC_GPIOINT->IO0IntStatR & (1 << i)) {
        return i + 32;
      }
    } else {
      if (LPC_GPIOINT->IO2IntStatF & (1 << i))
        return i;
      else if (LPC_GPIOINT->IO2IntStatR & (1 << i))
        return i + 32;
    }
  }
}

void clear_pin_interrupt(uint32_t port_num, uint32_t pin) {
  if (port_num == 0)
    // LPC_GPIOINT->IO0IntClr |= (1 << pin);
    LPC_GPIOINT->IO0IntClr = 0xffffffff;
  else
    // LPC_GPIOINT->IO2IntClr |= (1 << pin);
    LPC_GPIOINT->IO2IntClr = 0xffffffff;
}