// //-----------------------------------------------------------------------------------------------------

// #include "FreeRTOS.h"
// #include "board_io.h"
// #include "common_macros.h"
// #include "periodic_scheduler.h"
// #include "sj2_cli.h"
// #include "task.h"
// #include <stdio.h>

// #include "i2c_slave_functions.h"
// #include "i2c_slave_init.h"
// uint8_t volatile slave_array_memory[256];

// /* -------------- Write data to Slave AKA slave is the receiver ------------- */
// bool i2c_slave_callback__write_memory(uint8_t memory_index, uint8_t memory_value) {
//   bool write_status;
//   if (memory_index > 256 || memory_index < 0) {
//     fprintf(stderr, "Out of bound memory");
//     write_status = false;
//   } else {
//     // fprintf(stderr, "Wrote successfully\n");
//     slave_array_memory[memory_index] = memory_value;
//     write_status = true;
//   }
//   return write_status;
// }

// /* ------------ Read data from Slave AKA slave is the transmitter ----------- */
// bool i2c_slave_callback__read_memory(uint8_t memory_index, uint8_t *memory) {
//   bool read_status;
//   if (memory_index > 256 || memory_index < 0) {
//     fprintf(stderr, "Out of bound memory");
//     read_status = false;
//   } else {
//     *memory = slave_array_memory[memory_index];
//     read_status = true;
//   }
//   return read_status;
// }

// void turn_on_an_led() { LPC_GPIO1->PIN |= (1 << 18); }

// void turn_off_an_led() { LPC_GPIO1->PIN &= ~(1 << 18); }

// int main(void) {
//   sj2_cli__init();
//   LPC_IOCON->P0_10 |= (1 << 10);
//   LPC_IOCON->P0_11 |= (1 << 10);
//   gpio__construct_with_function(0, 10, 2);
//   gpio__construct_with_function(0, 11, 2);
//   i2c_slave_init(0x86);

//   LPC_IOCON->P1_18 &= ~(0b111 << 0);
//   LPC_GPIO1->DIR |= (1 << 18);

//   while (1) {
//     if (slave_array_memory[0] == 0) {
//       turn_off_an_led();
//     } else {
//       turn_on_an_led();
//     }
//   }

//   return 0;
// }

//-----------------------------------------------------------------------------------------------------

// // Watchdogs lab assignment
// #include "FreeRTOS.h"
// #include "acceleration.h"
// #include "cli_handlers.h"
// #include "event_groups.h"
// #include "ff.h"
// #include "sd_card.h"
// #include <queue.h>
// #include <stdio.h>
// #include <string.h>
// #include <task.h>

// #define BIT_0 (1 << 0)
// #define BIT_1 (1 << 1)

// static QueueHandle_t lab0_queue;
// EventGroupHandle_t checkin;

// void write_file_using_fatfs_pi(uint32_t value) {
//   const char *filename = "Watchdogs_lab.txt";
//   FIL file; // File handle
//   UINT bytes_written = 0;
//   FRESULT result = f_open(&file, filename, (FA_WRITE | FA_OPEN_APPEND));
//   uint16_t time = xTaskGetTickCount();

//   if (FR_OK == result) {
//     char string[64];
//     sprintf(string, "Time: %i, Value: %i\n", time, value);
//     if (FR_OK == f_write(&file, string, strlen(string), &bytes_written)) {
//     } else {
//       printf("ERROR: Failed to write data to file\n");
//     }
//     f_close(&file);
//   } else {
//     printf("ERROR: Failed to open: %s\n", filename);
//   }
// }

// void producer(void *p) {
//   if (acceleration__init()) {
//     fprintf(stderr, "Sensor init successfully.\n");
//   } else {
//     fprintf(stderr, "Sensor init failed.\n");
//   }

//   int count = 0;
//   acceleration__axis_data_s data = {0};
//   uint32_t value = 0;
//   bool flag = false;

//   while (1) {
//     while (count != 100) {
//       // printf("Producer task.\n");
//       data = acceleration__get_data();
//       // fprintf(stderr, "Data = %i\n", data.z);
//       value += data.z;
//       // fprintf(stderr, "Value = %i\n", value);
//       count++;
//       flag = true;
//     }

//     if (flag) {
//       value = value / 100;
//       // fprintf(stderr, "Avg after 100ms = %i\n\n", value);
//       xQueueSend(lab0_queue, &value, 0);
//       flag = false;
//       count = 0;
//     }

//     xEventGroupSetBits(checkin, BIT_0);
//     vTaskDelay(1000);
//   }
// }

// void consumer(void *p) {
//   uint32_t value = 0;
//   while (1) {
//     // printf("Consumer task.\n");
//     xQueueReceive(lab0_queue, &value, portMAX_DELAY);
//     write_file_using_fatfs_pi(value);
//     xEventGroupSetBits(checkin, BIT_1);
//   }
// }

// void watchdog_task(void *params) {
//   EventBits_t uxBits;

//   while (1) {
//     uxBits = xEventGroupWaitBits(checkin, BIT_0 | BIT_1, pdTRUE, pdFALSE, 2000);

//     if ((uxBits & (BIT_0 | BIT_1)) == (BIT_0 | BIT_1)) {
//       fprintf(stderr, "Both tasks've checked in.\n");
//     } else if ((uxBits & BIT_0) != 0) {
//       fprintf(stderr, "Producer checked in.\n");
//     } else if ((uxBits & BIT_1) != 0) {
//       fprintf(stderr, "Consumer checked in.\n");
//     } else {
//       fprintf(stderr, "Both tasks failed.\n");
//     }
//   }
// }

// void main(void) {
//   sj2_cli__init();
//   lab0_queue = xQueueCreate(1, sizeof(int));
//   checkin = xEventGroupCreate();

//   xTaskCreate(producer, "producer", 2048 / sizeof(void *), NULL, PRIORITY_MEDIUM, NULL);
//   xTaskCreate(consumer, "consumer", 2048 / sizeof(void *), NULL, PRIORITY_MEDIUM, NULL);
//   xTaskCreate(watchdog_task, "watchdog", 2048 / sizeof(void *), NULL, PRIORITY_HIGH, NULL);
//   vTaskStartScheduler();
// }

//-----------------------------------------------------------------------------------------------------

// // Producer Consumer Tasks
// #include "FreeRTOS.h"
// #include "cli_handlers.h"
// #include <gpio_lab.h>
// #include <queue.h>
// #include <stdio.h>
// #include <task.h>

// static QueueHandle_t switch_queue;

// typedef enum { switch__off, switch__on } switch_e;

// switch_e get_switch_input_from_switch0() {
//   if (gpio0__get_level(0, 29)) {
//     return switch__on;
//   }

//   return switch__off;
// }

// // TODO: Create this task at PRIORITY_LOW
// void producer(void *p) {
//   while (1) {
//     // This xQueueSend() will internally switch context to "consumer" task because it is higher priority than this
//     // "producer" task Then, when the consumer task sleeps, we will resume out of xQueueSend()and go over to the next
//     // line

//     // TODO: Get some input value from your board
//     const switch_e switch_value = get_switch_input_from_switch0();

//     // TODO: Print a message before xQueueSend()
//     // Note: Use printf() and not fprintf(stderr, ...) because stderr is a polling printf
//     printf("Producer task.\n");
//     xQueueSend(switch_queue, &switch_value, 0);
//     // TODO: Print a message after xQueueSend()
//     printf("After xQueueSend()\n");

//     vTaskDelay(1000);
//   }
// }

// // TODO: Create this task at PRIORITY_HIGH
// void consumer(void *p) {
//   switch_e switch_value;
//   while (1) {
//     // TODO: Print a message before xQueueReceive()
//     printf("Before xQueueReceive()\n");
//     if (xQueueReceive(switch_queue, &switch_value, portMAX_DELAY)) {
//       // TODO: Print a message after xQueueReceive()
//       if (switch_value == switch__on) {
//         printf("switch_on.\n\n");
//       } else {
//         printf("switch_off.\n\n");
//       }
//     }
//   }
// }

// void main(void) {
//   sj2_cli__init();

//   LPC_IOCON->P0_29 &= ~(0b111 << 0);
//   gpio0__set_as_input(0, 29);

//   // TODO: Create your tasks
//   xTaskCreate(producer, "producer", 2048 / sizeof(void *), NULL, PRIORITY_LOW, NULL);
//   xTaskCreate(consumer, "consumer", 2048 / sizeof(void *), NULL, PRIORITY_HIGH, NULL);

//   // TODO Queue handle is not valid until you create it
//   switch_queue =
//       xQueueCreate(1, sizeof(switch_e)); // Choose depth of item being our enum (1 should be okay for this example)

//   vTaskStartScheduler();
// }

//-----------------------------------------------------------------------------------------------------

// // UART lab
// #include "FreeRTOS.h"
// #include "task.h"
// #include <stdio.h>
// #include <string.h>

// #include "uart_lab.h"

// void uart_pin_configuration(uart_number_e uart) {
//   if (uart == UART_2) {
//     // puts("UART2 pins config.\n");
//     LPC_IOCON->P0_10 &= ~(0b111);
//     LPC_IOCON->P0_10 |= (0b001); // U2_TXD
//     LPC_IOCON->P0_11 &= ~(0b111);
//     LPC_IOCON->P0_11 |= (0b001); // U2_RXD
//   } else if (uart == UART_3) {
//     // puts("UART3 pins config.\n");
//     LPC_IOCON->P4_28 &= ~(0b111);
//     LPC_IOCON->P4_28 |= (0b010); // U3_TXD
//     LPC_IOCON->P4_29 &= ~(0b111);
//     LPC_IOCON->P4_29 |= (0b010); // U3_RXD
//   }
// }

// // Part 1 READ task
// void uart_read_task(void *p) {
//   while (1) {
//     // TODO: Use uart_lab__polled_get() function and printf the received value
//     char read_byte;
//     if (uart_lab__polled_get(UART_2, &read_byte)) {
//       fprintf(stderr, "The data is: %c\n", read_byte);
//     }
//   }
//   vTaskDelay(500);
// }

// // // Part 2 READ task
// // void uart_read_task(void *p) {
// //   while (1) {
// //     char read_byte = 0;
// //     uart_lab__get_char_from_queue(&read_byte, portMAX_DELAY);
// //     fprintf(stderr, "The data is: %c\n", read_byte);
// //   }
// //   vTaskDelay(500);
// // }

// // Write task
// void uart_write_task(void *p) {
//   while (1) {
//     // TODO: Use uart_lab__polled_put() function and send a value
//     if (uart_lab__polled_put(UART_2, 'u')) {
//       ;
//     }
//     vTaskDelay(500);
//   }
// }

// // Part 3
// // This task is done for you, but you should understand what this code is doing
// void board_1_sender_task(void *p) {
//   char number_as_string[16] = {0};

//   while (true) {
//     const int number = rand();
//     sprintf(number_as_string, "%i", number);

//     // Send one char at a time to the other board including terminating NULL char
//     for (int i = 0; i <= strlen(number_as_string); i++) {
//       uart_lab__polled_put(UART_3, number_as_string[i]);
//       printf("Sent: %c\n", number_as_string[i]);
//       vTaskDelay(10);
//     }

//     printf("Sent: %i over UART to the other board\n", number);
//     vTaskDelay(10000);
//   }
// }

// void board_2_receiver_task(void *p) {
//   char number_as_string[16] = {0};
//   int counter = 0;

//   while (true) {
//     char byte = 0;
//     uart_lab__get_char_from_queue(&byte, portMAX_DELAY);
//     printf("Received: %c\n", byte);

//     // This is the last char, so print the number
//     if ('\0' == byte) {
//       number_as_string[counter] = '\0';
//       counter = 0;
//       printf("Received this number from the other board: %s\n", number_as_string);
//     }
//     // We have not yet received the NULL '\0' char, so buffer the data
//     else {
//       // TODO: Store data to number_as_string[] array one char at a time
//       // Hint: Use counter as an index, and increment it as long as we do not reach max value of 16
//       if (counter < 16) {
//         number_as_string[counter] = byte;
//         // fprintf(stderr, "Data is: %c\n", number_as_string[counter]);
//         counter++;
//       }
//     }
//   }
// }

// // Main()
// void main(void) {
//   // TODO: Use uart_lab__init() function and initialize UART2 or UART3 (your choice)
//   uart_lab__init(UART_3, 96, 115200);

//   // TODO: Pin Configure IO pins to perform UART2/UART3 function
//   uart_pin_configuration(UART_3);

//   // Part 2
//   uart__enable_receive_interrupt(UART_3);

//   // // Part 1 & 2 xTaskCreate()
//   // xTaskCreate(uart_read_task, "read", 2048 / sizeof(void *), NULL, 1, NULL);
//   // xTaskCreate(uart_write_task, "write", 2048 / sizeof(void *), NULL, 1, NULL);

//   // Part 3 xTaskCreate()
//   // xTaskCreate(board_2_receiver_task, "receiver", 2048 / sizeof(void *), NULL, 1, NULL);
//   // xTaskCreate(board_1_sender_task, "sender", 2048 / sizeof(void *), NULL, 1, NULL);

//   vTaskStartScheduler();
// }

//-----------------------------------------------------------------------------------------------------

// // SPI LAB PART 1 & 2 & 3
// #include "FreeRTOS.h"
// #include "task.h"
// #include <stdio.h>

// #include "ssp2.h"
// #include "ssp2_lab.h"

// #include "semphr.h"

// // In main(), initialize your Mutex:
// SemaphoreHandle_t spi_bus_mutex;

// /**
//  * Adesto flash asks to send 24-bit address
//  * We can use our usual uint32_t to store the address
//  * and then transmit this address over the SPI driver
//  * one byte at a time
//  */
// void adesto_flash_send_address(uint32_t address);

// void todo_configure_your_ssp2_pin_functions();
// void gpio_set_ssp2_function(uint32_t *ptr);

// // TODO: Implement Adesto flash memory CS signal as a GPIO driver
// void adesto_cs(void); // Chip select
// void adesto_ds(void); // Chip deselect

// // TODO: Study the Adesto flash 'Manufacturer and Device ID' section
// typedef struct {
//   uint8_t manufacturer_id;
//   uint8_t device_id_1;
//   uint8_t device_id_2;
//   uint8_t extended_device_id;
// } adesto_flash_id_s;

// // Extra credit
// void adesto_erase() {
//   adesto_cs();

//   uint8_t op_code = 0x60;
//   ssp2__exchange_byte(op_code);
//   vTaskDelay(10);

//   adesto_ds();
// }

// adesto_flash_id_s adesto_read(uint32_t read_addr) {
//   adesto_flash_id_s data = {0};
//   adesto_cs();

//   uint8_t op_code = 0x03;
//   ssp2__exchange_byte(op_code);
//   adesto_flash_send_address(read_addr);
//   data.extended_device_id = ssp2__exchange_byte(1);

//   adesto_ds();

//   return data;
// }

// void adesto_write(uint32_t address, uint8_t data) {
//   adesto_cs();
//   uint8_t write_en = 0x06;
//   ssp2__exchange_byte(write_en);
//   adesto_ds();

//   adesto_cs();
//   uint8_t op_code = 0x02;
//   ssp2__exchange_byte(op_code);
//   adesto_flash_send_address(address);
//   ssp2__exchange_byte(data);
//   vTaskDelay(10);
//   adesto_ds();

//   adesto_cs();
//   uint8_t write_dis = 0x04;
//   ssp2__exchange_byte(write_dis);
//   adesto_ds();
// }

// void spi_read_and_write_task(void *p) {
//   const uint32_t spi_clock_mhz = 24;
//   ssp2__init(spi_clock_mhz);

//   todo_configure_your_ssp2_pin_functions();

//   while (1) {
//     uint32_t addr = 0x0007FC00;
//     uint8_t data = 0x48;
//     adesto_write(addr, data);
//     adesto_flash_id_s id = adesto_read(addr);
//     fprintf(stderr, "Data: %02xh\n", id.extended_device_id);

//     vTaskDelay(500);
//   }
// }

// // Chip select
// void adesto_cs() {
//   // Send opcode and read bytes
//   // TODO: Populate members of the 'adesto_flash_id_s' struct
//   LPC_GPIO1->PIN &= ~(1 << 10);
// }

// // Chip deselect
// void adesto_ds() { LPC_GPIO1->PIN |= (1 << 10); }

// // TODO: Implement the code to read Adesto flash memory signature
// // TODO: Create struct of type 'adesto_flash_id_s' and return it
// adesto_flash_id_s adesto_read_signature(void) {
//   adesto_flash_id_s data = {0};

//   adesto_cs();

//   uint8_t op_code = 0x9f;
//   ssp2__exchange_byte(op_code);
//   data.manufacturer_id = ssp2__exchange_byte_lab(0);
//   data.device_id_1 = ssp2__exchange_byte_lab(0);
//   data.device_id_2 = ssp2__exchange_byte_lab(0);

//   adesto_ds();

//   return data;
// }

// void spi_task(void *p) {
//   const uint32_t spi_clock_mhz = 24;
//   ssp2__init(spi_clock_mhz);

//   // From the LPC schematics pdf, find the pin numbers connected to flash memory
//   // Read table 84 from LPC User Manual and configure PIN functions for SPI2 pins
//   // You can use gpio__construct_with_function() API from gpio.h
//   //
//   // Note: Configure only SCK2, MOSI2, MISO2.
//   // CS will be a GPIO output pin(configure and setup direction)
//   todo_configure_your_ssp2_pin_functions();

//   while (1) {
//     adesto_flash_id_s id = adesto_read_signature();
//     // TODO: printf the members of the 'adesto_flash_id_s' struct
//     fprintf(stderr, "Manufacturer id: %02xh\n", id.manufacturer_id);
//     fprintf(stderr, "Device id 1: %02xh\n", id.device_id_1);
//     fprintf(stderr, "Device id 2: %02xh\n\n", id.device_id_2);

//     vTaskDelay(500);
//   }
// }

// // void spi_id_verification_task(void *p) {
// //   while (1) {
// //     adesto_flash_id_s id = adesto_read_signature();

// //     // When we read a manufacturer ID we do not expect, we will kill this task
// //     if (0x1F != id.manufacturer_id) {
// //       // fprintf(stderr, "Manufacturer ID read failure\n");
// //       fprintf(stderr, "Manufacturer ID read failure where id = %xh\n", id.manufacturer_id);
// //       vTaskSuspend(NULL); // Kill this task
// //     }
// //   }
// // }

// void spi_id_verification_task(void *p) {
//   while (1) {
//     if (xSemaphoreTake(spi_bus_mutex, 1000)) {
//       // Use Guarded Resource
//       adesto_flash_id_s id = adesto_read_signature();

//       // When we read a manufacturer ID we do not expect, we will kill this task
//       if (0x1F != id.manufacturer_id) {
//         fprintf(stderr, "Manufacturer ID read failure\n");
//         vTaskSuspend(NULL); // Kill this task
//       }
//       fprintf(stderr, "Manufacturer ID = %xh\n", id.manufacturer_id);
//       // Give Semaphore back:
//       xSemaphoreGive(spi_bus_mutex);
//     }
//   }
// }

// void main(void) {
//   spi_bus_mutex = xSemaphoreCreateMutex();

//   // TODO: Initialize your SPI, its pins, Adesto flash CS GPIO etc...
//   const uint32_t spi_clock_mhz = 24;
//   ssp2__init(spi_clock_mhz);
//   todo_configure_your_ssp2_pin_functions();

//   // Part 2
//   // Create two tasks that will continously read signature
//   // xTaskCreate(spi_id_verification_task_1, "task_1", 2048 / sizeof(void *), NULL, 1, NULL);
//   // xTaskCreate(spi_id_verification_task_2, "task_2", 2048 / sizeof(void *), NULL, 1, NULL);

//   // Part 1
//   xTaskCreate(spi_task, "SPI TASK", 2048 / sizeof(void *), NULL, 1, NULL);

//   // Extra credit
//   // xTaskCreate(spi_read_and_write_task, "write", 2048 / sizeof(void *), NULL, 1, NULL);
//   vTaskStartScheduler();
// }

// void gpio_set_ssp2_function(uint32_t *ptr) {
//   *ptr &= ~(0b111 << 0);
//   *ptr |= (0b100 << 0);
// }

// void todo_configure_your_ssp2_pin_functions() {
//   gpio_set_ssp2_function(&LPC_IOCON->P1_0); // Set SCK2
//   gpio_set_ssp2_function(&LPC_IOCON->P1_1); // Set MOSI2
//   gpio_set_ssp2_function(&LPC_IOCON->P1_4); // Set MISO2

//   LPC_IOCON->P1_10 &= ~(0b111 << 0); // Leave as GPIO
//   LPC_GPIO1->DIR |= (1 << 10);       // Set as output pin
// }

// void adesto_flash_send_address(uint32_t address) {
//   (void)ssp2__exchange_byte((address >> 16) & 0xFF);
//   (void)ssp2__exchange_byte((address >> 8) & 0xFF);
//   (void)ssp2__exchange_byte((address >> 0) & 0xFF);
// }

//-----------------------------------------------------------------------------------------------------

// // Lab ADC &PWM Part 2 & 3
// #include "FreeRTOS.h"
// #include "adc.h"
// #include "gpio.h"
// #include "pwm1.h"
// #include "queue.h"
// #include "task.h"
// #include <stdio.h>

// void pin_configure_channel_as_io_pin(gpio__port_e, uint8_t, gpio__function_e);

// // This is the queue handle we will need for the xQueue Send/Receive API
// static QueueHandle_t adc_to_pwm_task_queue;

// void adc_task(void *p) {
//   // NOTE: Reuse the code from Part 1
//   adc__initialize();

//   adc__enable_burst_mode();

//   pin_configure_channel_as_io_pin(GPIO__PORT_1, 31, GPIO__FUNCTION_3);

//   int adc_reading = 0; // Note that this 'adc_reading' is not the same variable as the one from adc_task
//   while (1) {
//     // Implement code to send potentiometer value on the queue
//     // a) read ADC input to 'int adc_reading'
//     adc_reading = adc__get_channel_reading_with_burst_mode(ADC__CHANNEL_5);
//     // b) Send to queue: xQueueSend(adc_to_pwm_task_queue, &adc_reading, 0);
//     xQueueSend(adc_to_pwm_task_queue, &adc_reading, 0);
//     vTaskDelay(1000);
//   }
// }

// void pwm_task(void *p) {
//   // NOTE: Reuse the code from Part 0
//   pwm1__init_single_edge(1000);
//   pin_configure_channel_as_io_pin(GPIO__PORT_2, 0, GPIO__FUNCTION_1);
//   int adc_reading = 0;
//   uint8_t percent = 0;

//   while (1) {
//     // Implement code to receive potentiometer value from queue
//     if (xQueueReceive(adc_to_pwm_task_queue, &adc_reading, 100)) {
//       fprintf(stderr, "ADC value received: %d\n", adc_reading);
//       percent = (adc_reading * 100) / 4095;
//       fprintf(stderr, "The percentage is: %d\n", percent);
//       printf("MR1: %d\n", LPC_PWM1->MR1);
//       pwm1__set_duty_cycle(PWM1__2_0, percent);
//     }

//     // We do not need task delay because our queue API will put task to sleep when there is no data in the queue
//     // vTaskDelay(100);
//   }
// }

// void main(void) {
//   // Queue will only hold 1 integer
//   adc_to_pwm_task_queue = xQueueCreate(1, sizeof(int));

//   xTaskCreate(adc_task, "adc", 2048 / sizeof(void *), NULL, 1, NULL);
//   xTaskCreate(pwm_task, "pwm", 2048 / sizeof(void *), NULL, 1, NULL);
//   vTaskStartScheduler();
// }

// void pin_configure_channel_as_io_pin(gpio__port_e port_num, uint8_t pin_num, gpio__function_e func) {
//   if (func == GPIO__FUNCTION_3) {
//     LPC_IOCON->P1_31 &= ~(1 << 7);
//   }

//   gpio_s tempt = gpio__construct_with_function(port_num, pin_num, func);
// }

//-----------------------------------------------------------------------------------------------------

// // Lab ADC &PWM Part 1
// #include "adc.h"

// #include "FreeRTOS.h"
// #include "gpio.h"
// #include "task.h"

// #include <stdio.h>

// void pin_configure_adc_channel_as_io_pin(gpio__port_e, uint8_t, gpio__function_e);

// void adc_task(void *p) {
//   adc__initialize();

//   // TODO This is the function you need to add to adc.h
//   // You can configure burst mode for just the channel you are using
//   adc__enable_burst_mode();

//   // Configure a pin, such as P1.31 with FUNC 011 to route this pin as ADC channel 5
//   // You can use gpio__construct_with_function() API from gpio.h
//   pin_configure_adc_channel_as_io_pin(GPIO__PORT_1, 31, GPIO__FUNCTION_3); // TODO You need to write this function

//   while (1) {
//     // Get the ADC reading using a new routine you created to read an ADC burst reading
//     // TODO: You need to write the implementation of this function
//     const uint16_t adc_value = adc__get_channel_reading_with_burst_mode(5);
//     fprintf(stderr, "value: %d V\n", adc_value);
//     vTaskDelay(100);
//   }
// }

// void main(void) {
//   xTaskCreate(adc_task, "adc", 2048 / sizeof(void *), NULL, 1, NULL);
//   vTaskStartScheduler();
// }

// void pin_configure_adc_channel_as_io_pin(gpio__port_e port_num, uint8_t pin_num, gpio__function_e func) {
//   gpio_s tempt = gpio__construct_with_function(port_num, pin_num, func);
//   LPC_IOCON->P1_31 &= ~(1 << 7);
// }

//-----------------------------------------------------------------------------------------------------

// // Lab ADC &PWM Part 0
// #include "pwm1.h"

// #include "FreeRTOS.h"
// #include "gpio.h"
// #include "task.h"

// void pin_configure_pwm_channel_as_io_pin(gpio__port_e, uint8_t, gpio__function_e);

// void pwm_task(void *p) {
//   pwm1__init_single_edge(1000);

//   // Locate a GPIO pin that a PWM channel will control
//   // NOTE You can use gpio__construct_with_function() API from gpio.h
//   // TODO Write this function yourself
//   pin_configure_pwm_channel_as_io_pin(GPIO__PORT_2, 0, GPIO__FUNCTION_1);

//   // We only need to set PWM configuration once, and the HW will drive
//   // the GPIO at 1000Hz, and control set its duty cycle to 50%
//   pwm1__set_duty_cycle(PWM1__2_0, 50);

//   // Continue to vary the duty cycle in the loop
//   uint8_t percent = 0;
//   while (1) {
//     pwm1__set_duty_cycle(PWM1__2_0, percent);

//     if (++percent > 100) {
//       percent = 0;
//     }

//     vTaskDelay(100);
//   }
// }

// void main(void) {
//   xTaskCreate(pwm_task, "pwm", 2048 / sizeof(void *), NULL, 1, NULL);
//   vTaskStartScheduler();
// }

// void pin_configure_pwm_channel_as_io_pin(gpio__port_e port_num, uint8_t pin_num, gpio__function_e func) {
//   gpio_s tempt = gpio__construct_with_function(port_num, pin_num, func);
// }

//-----------------------------------------------------------------------------------------------------

// Lab INTERRUPT
// #include "FreeRTOS.h"
// #include "gpio_lab.h"
// #include "irs_lab.h"
// #include "lpc40xx.h"
// #include "lpc_peripherals.h"
// #include "semphr.h"
// #include <stdio.h>

// void sleep_on_sem_task(void *);
// void sleep_on_sem_task_1(void *);

// static SemaphoreHandle_t switch_pressed_signal;
// static SemaphoreHandle_t switch_pressed_signal_1;

// // Objective of the assignment is to create a clean API to register sub-interrupts like so:
// void pin_isr(void) {
//   fprintf(stderr, "ISR Entry 1\n");
//   xSemaphoreGiveFromISR(switch_pressed_signal, NULL);
// }

// void pin_isr_1(void) {
//   fprintf(stderr, "ISR Entry 2\n");
//   xSemaphoreGiveFromISR(switch_pressed_signal_1, NULL);
// }

// void main(void) {
//   switch_pressed_signal = xSemaphoreCreateBinary();
//   switch_pressed_signal_1 = xSemaphoreCreateBinary();

//   gpio0__attach_interrupt(29, GPIO_INTR__FALLING_EDGE, pin_isr);
//   gpio0__attach_interrupt(30, GPIO_INTR__RISING_EDGE, pin_isr_1);
//   NVIC_EnableIRQ(GPIO_IRQn); // Enable interrupt gate for the GPIO
//   lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__GPIO, gpio0__interrupt_dispatcher, "name");

//   xTaskCreate(sleep_on_sem_task, "sem", (512U * 4) / sizeof(void *), NULL, PRIORITY_LOW, NULL);
//   xTaskCreate(sleep_on_sem_task_1, "sem", (512U * 4) / sizeof(void *), NULL, PRIORITY_LOW, NULL);
//   vTaskStartScheduler();
// }

// void sleep_on_sem_task(void *p) {
//   while (1) {
//     // Use xSemaphoreTake with forever delay and blink an LED when you get the signal
//     if (xSemaphoreTake(switch_pressed_signal, portMAX_DELAY)) {
//       fprintf(stderr, "inside semaphore 1.\n");
//       gpio0__set_high(1, 24);
//       vTaskDelay(50);
//       gpio0__set_low(1, 24);
//     }
//   }
// }

// void sleep_on_sem_task_1(void *p) {
//   while (1) {
//     // Use xSemaphoreTake with forever delay and blink an LED when you get the signal
//     if (xSemaphoreTake(switch_pressed_signal_1, portMAX_DELAY)) {
//       fprintf(stderr, "inside semaphore 2.\n");
//       gpio0__set_high(1, 24);
//       vTaskDelay(50);
//       gpio0__set_low(1, 24);
//     }
//   }
// }

//-----------------------------------------------------------------------------------------------------

// #include "FreeRTOS.h"
// #include "lpc_peripherals.h"
// #include "semphr.h"
// #include <stdio.h>

// #include "lpc40xx.h"

// void configure_your_gpio_interrupt();
// void clear_gpio_interrupt();
// void sleep_on_sem_task(void *);

// static SemaphoreHandle_t switch_pressed_signal;

// void main(void) {
//   switch_pressed_signal = xSemaphoreCreateBinary(); // Create your binary semaphore

//   configure_your_gpio_interrupt(); // TODO: Setup interrupt by re-using code from Part 0
//   NVIC_EnableIRQ(GPIO_IRQn);       // Enable interrupt gate for the GPIO

//   xTaskCreate(sleep_on_sem_task, "sem", (512U * 4) / sizeof(void *), NULL, PRIORITY_LOW, NULL);
//   vTaskStartScheduler();
// }

// // WARNING: You can only use printf(stderr, "foo") inside of an ISR
// void gpio_interrupt(void) {
//   fprintf(stderr, "ISR Entry");
//   xSemaphoreGiveFromISR(switch_pressed_signal, NULL);
//   clear_gpio_interrupt();
// }

// void sleep_on_sem_task(void *p) {
//   while (1) {
//     // Use xSemaphoreTake with forever delay and blink an LED when you get the signal
//     if (xSemaphoreTake(switch_pressed_signal, portMAX_DELAY)) {
//       LPC_GPIO1->PIN ^= (1 << 24);
//     }
//   }
// }

// void configure_your_gpio_interrupt() {
//   LPC_IOCON->P0_30 &= ~(0b10111 << 0);
//   LPC_GPIO0->DIR &= ~(1 << 30);
//   LPC_GPIOINT->IO0IntEnF |= (1 << 30);
//   lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__GPIO, gpio_interrupt, "name");
// }

// void clear_gpio_interrupt() { LPC_GPIOINT->IO0IntClr |= (1 << 30); }

//-----------------------------------------------------------------------------------------------------

// #include "lpc40xx.h"
// #include "lpc_peripherals.h"
// #include <stdio.h>

// void gpio_interrupt(void);

// // Step 1:
// void main(void) {
//   // Read Table 95 in the LPC user manual and setup an interrupt on a switch connected to Port0 or Port2
//   // a) For example, choose SW2 (P0_30) pin on SJ2 board and configure as input
//   //.  Warning: P0.30, and P0.31 require pull-down resistors
//   LPC_IOCON->P0_30 &= ~(0b10111 << 0);
//   LPC_GPIO0->DIR &= ~(1 << 30);
//   // b) Configure the registers to trigger Port0 interrupt (such as falling edge)
//   LPC_GPIOINT->IO0IntEnF |= (1 << 30); // enable 1 value for P0_30 (EnF for falling, EnR for rising)

//   // Install GPIO interrupt function at the CPU interrupt (exception) vector
//   // c) Hijack the interrupt vector at interrupt_vector_table.c and have it call our gpio_interrupt()
//   //    Hint: You can declare 'void gpio_interrupt(void)' at interrupt_vector_table.c such that it can see this
//   function

//   // Most important step: Enable the GPIO interrupt exception using the ARM Cortex M API (this is from lpc40xx.h)
//   NVIC_EnableIRQ(GPIO_IRQn);

//   // Toggle an LED in a loop to ensure/test that the interrupt is entering ane exiting
//   // For example, if the GPIO interrupt gets stuck, this LED will stop blinking
//   while (1) {
//     delay__ms(100);
//     // TODO: Toggle an LED here
//     LPC_GPIO1->PIN ^= (1 << 24);
//   }
// }

// // Step 2:
// void gpio_interrupt(void) {
//   // a) Clear Port0/2 interrupt using CLR0 or CLR2 registers
//   LPC_GPIOINT->IO0IntClr |= (1 << 30);
//   // b) Use fprintf(stderr) or blink and LED here to test your ISR
//   fprintf(stderr, "button pressed.");
// }

//-----------------------------------------------------------------------------------------------------

// #include "FreeRTOS.h"
// #include "gpio_lab.h"
// #include "semphr.h"
// #include "task.h"
// #include <stdbool.h>
// #include <stdio.h>

// typedef struct {
//   uint8_t port;
//   uint8_t pin;
// } port_pin_s;

// static SemaphoreHandle_t switch_press_indication;

// void led_task(void *task_parameter) {
//   const port_pin_s *led = (port_pin_s *)task_parameter;

//   while (true) {
//     // Note: There is no vTaskDelay() here, but we use sleep mechanism while waiting for the binary semaphore
//     if (xSemaphoreTake(switch_press_indication, 1000)) {
//       fprintf(stderr, "AAAAAAAAAAAA");
//       gpio0__set_high(led->port, led->pin);
//       vTaskDelay(200);
//     } else {
//       puts("Timeout: No switch press indication for 1000ms");
//     }
//     gpio0__set_low(led->port, led->pin);
//     vTaskDelay(200);
//   }
// }

// void switch_task(void *task_parameter) {
//   const port_pin_s *sw = (port_pin_s *)task_parameter;

//   while (true) {
//     // TODO: If switch pressed, set the binary semaphore
//     if (gpio0__get_level(sw->port, sw->pin)) {
//       fprintf(stderr, "BBBBBBBBBBBBB");
//       xSemaphoreGive(switch_press_indication);
//     }

//     // Task should always sleep otherwise they will use 100% CPU
//     // This task sleep also helps avoid spurious semaphore give during switch debeounce
//     vTaskDelay(100);
//   }
// }

// int main(void) {
//   switch_press_indication = xSemaphoreCreateBinary();

//   // Hint: Use on-board LEDs first to get this logic to work
//   //       After that, you can simply switch these parameters to off-board LED and a switch
//   static port_pin_s sw = {0, 29};
//   static port_pin_s led = {1, 18};

//   gpio0__set_as_input(sw.port, sw.pin);
//   gpio0__set_as_output(sw.port, sw.pin);

//   xTaskCreate(switch_task, "SW", 2048 / sizeof(void *), (void *)&sw, 1, NULL);
//   xTaskCreate(led_task, "LED", 2048 / sizeof(void *), (void *)&led, 1, NULL);

//   vTaskStartScheduler();

//   return 0;
// }

//-----------------------------------------------------------------------------------------------------

// #include "FreeRTOS.h"
// #include "gpio_lab.h"
// #include "task.h"
// #include <stdbool.h>
// #include <stdio.h>

// typedef struct {
//   uint8_t port;
//   uint8_t pin;
// } port_pin_s;

// void led_task(void *task_parameter) {
//   // Type-cast the paramter that was passed from xTaskCreate()
//   const port_pin_s *led = (port_pin_s *)(task_parameter);
//   while (true) {
//     gpio0__set_high(led->port, led->pin);
//     vTaskDelay(1000);

//     gpio0__set_low(led->port, led->pin);
//     vTaskDelay(1000);
//   }
// }

// int main(void) {
//   // TODO:
//   // Create two tasks using led_task() function
//   // Pass each task its own parameter:
//   // This is static such that these variables will be allocated in RAM and not go out of scope
//   static port_pin_s led0 = {1, 18};
//   static port_pin_s led1 = {1, 24};

//   xTaskCreate(led_task, "1", 2048 / sizeof(void *), &led0, 1, NULL); /* &led0 is a task parameter going to led_task
//                                                                       */
//   xTaskCreate(led_task, "2", 2048 / sizeof(void *), &led1, 2, NULL);

//   vTaskStartScheduler();
//   return 0;
// }

//-----------------------------------------------------------------------------------------------------

// //const uint32_t led3 = (1U << 18);

// void led_task(void *pvParameters) {
//   // Choose one of the onboard LEDS by looking into schematics and write code for the below
//   // 0) Set the IOCON MUX function(if required) select pins to 000
//   LPC_IOCON->P1_18 &= ~(0b111 << 0);

//   // 1) Set the DIR register bit for the LED port pin
//   LPC_GPIO1->DIR |= (1<<18);

//   while (true) {
//     // 2) Set PIN register bit to 0 to turn ON LED (led may be active low)
//     LPC_GPIO1->PIN |= (1<<18);
//     vTaskDelay(2000);

//     // 3) Set PIN register bit to 1 to turn OFF LED
//     LPC_GPIO1->PIN &= ~(1<<18);
//     vTaskDelay(2000);
//   }
// }

// int main(void) {
//   // Create FreeRTOS LED task
//   xTaskCreate(led_task, "led", 2048 / sizeof(void *), NULL, PRIORITY_LOW, NULL);

//   vTaskStartScheduler();
//   return 0;
// }

//-----------------------------------------------------------------------------------------------------

// #include "FreeRTOS.h"
// #include "board_io.h"
// #include "common_macros.h"
// #include "periodic_scheduler.h"
// #include "sj2_cli.h"
// #include "task.h"
// #include <stdio.h>

// // 'static' to make these functions 'private' to this file
// static void create_blinky_tasks(void);
// static void create_uart_task(void);
// static void blink_task(void *params);
// static void uart_task(void *params);
// static void task_one(void *task_parameter);
// static void task_two(void *task_parameter);

// int main(void) {
//   create_blinky_tasks();
//   create_uart_task();

//   // If you have the ESP32 wifi module soldered on the board, you can try uncommenting this code
//   // See esp32/README.md for more details
//   // uart3_init();                                                                     // Also include:  uart3_init.h
//   // xTaskCreate(esp32_tcp_hello_world_task, "uart3", 1000, NULL, PRIORITY_LOW, NULL); // Include esp32_task.h
//   xTaskCreate(task_one, "1", 4096 / sizeof(void *), NULL, 1, NULL);
//   xTaskCreate(task_two, "2", 4096 / sizeof(void *), NULL, 1, NULL);

//   puts("Starting RTOS");
//   vTaskStartScheduler(); // This function never returns unless RTOS scheduler runs out of memory and fails

//   return 0;
// }

// static void task_one(void *task_parameter) {
//   while (true) {
//     // Read existing main.c regarding when we should use fprintf(stderr...) in place of printf()
//     // For this lab, we will use fprintf(stderr, ...)
//     // fprintf(stderr, "AAAAAAAAAAAA");

//     // Sleep for 100ms
//     vTaskDelay(100);
//   }
// }

// static void task_two(void *task_parameter) {
//   while (true) {
//     // fprintf(stderr, "bbbbbbbbbbbb");
//     vTaskDelay(100);
//   }
// }

// static void create_blinky_tasks(void) {
//   /**
//    * Use '#if (1)' if you wish to observe how two tasks can blink LEDs
//    * Use '#if (0)' if you wish to use the 'periodic_scheduler.h' that will spawn 4 periodic tasks, one for each LED
//    */
// #if (1)
//   // These variables should not go out of scope because the 'blink_task' will reference this memory
//   static gpio_s led0, led1;

//   // If you wish to avoid malloc, use xTaskCreateStatic() in place of xTaskCreate()
//   static StackType_t led0_task_stack[512 / sizeof(StackType_t)];
//   static StackType_t led1_task_stack[512 / sizeof(StackType_t)];
//   static StaticTask_t led0_task_struct;
//   static StaticTask_t led1_task_struct;

//   led0 = board_io__get_led0();
//   led1 = board_io__get_led1();

//   xTaskCreateStatic(blink_task, "led0", ARRAY_SIZE(led0_task_stack), (void *)&led0, PRIORITY_LOW, led0_task_stack,
//                     &led0_task_struct);
//   xTaskCreateStatic(blink_task, "led1", ARRAY_SIZE(led1_task_stack), (void *)&led1, PRIORITY_LOW, led1_task_stack,
//                     &led1_task_struct);
// #else
//   periodic_scheduler__initialize();
//   UNUSED(blink_task);
// #endif
// }

// static void create_uart_task(void) {
//   // It is advised to either run the uart_task, or the SJ2 command-line (CLI), but not both
//   // Change '#if (0)' to '#if (1)' and vice versa to try it out
// #if (0)
//   // printf() takes more stack space, size this tasks' stack higher
//   xTaskCreate(uart_task, "uart", (512U * 8) / sizeof(void *), NULL, PRIORITY_LOW, NULL);
// #else
//   sj2_cli__init();
//   UNUSED(uart_task); // uart_task is un-used in if we are doing cli init()
// #endif
// }

// static void blink_task(void *params) {
//   const gpio_s led = *((gpio_s *)params); // Parameter was input while calling xTaskCreate()

//   // Warning: This task starts with very minimal stack, so do not use printf() API here to avoid stack overflow
//   while (true) {
//     gpio__toggle(led);
//     vTaskDelay(500);
//   }
// }

// // This sends periodic messages over printf() which uses system_calls.c to send them to UART0
// static void uart_task(void *params) {
//   TickType_t previous_tick = 0;
//   TickType_t ticks = 0;

//   while (true) {
//     // This loop will repeat at precise task delay, even if the logic below takes variable amount of ticks
//     vTaskDelayUntil(&previous_tick, 2000);

//     /* Calls to fprintf(stderr, ...) uses polled UART driver, so this entire output will be fully
//      * sent out before this function returns. See system_calls.c for actual implementation.
//      *
//      * Use this style print for:
//      *  - Interrupts because you cannot use printf() inside an ISR
//      *    This is because regular printf() leads down to xQueueSend() that might block
//      *    but you cannot block inside an ISR hence the system might crash
//      *  - During debugging in case system crashes before all output of printf() is sent
//      */
//     ticks = xTaskGetTickCount();
//     fprintf(stderr, "%u: This is a polled version of printf used for debugging ... finished in", (unsigned)ticks);
//     fprintf(stderr, " %lu ticks\n", (xTaskGetTickCount() - ticks));

//     /* This deposits data to an outgoing queue and doesn't block the CPU
//      * Data will be sent later, but this function would return earlier
//      */
//     ticks = xTaskGetTickCount();
//     printf("This is a more efficient printf ... finished in");
//     printf(" %lu ticks\n\n", (xTaskGetTickCount() - ticks));
//   }
// }