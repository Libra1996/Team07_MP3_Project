#include "cli_handlers.h"
#include "gpio.h"
#include "gpio_lab.h"
#include "spi0.h"
#include <stdio.h>
#include <string.h>

#define SCI_BASS_TREBLE 0x02 // BASS and Treble reg
#define SCI_CLOCKF 0x03      // Clock reg
#define SCI_VOL 0x0B         // Volume reg

static const uint8_t bass_treble[6] = {'0', '1', '2', '3', '4', '5'};
static const uint16_t BASS[6] = {0x0000, 0x0022, 0x0055, 0x0088, 0x00BB, 0x00EE};
static const uint16_t TREBLE[6] = {0x0000, 0x2200, 0x5500, 0x8800, 0xBB00, 0xEE00};

static const uint16_t VOLUME[10] = {0xFEFE, 0X7D7D, 0X6464, 0x4B4B, 0x3C3C, 0x3535, 0x3030, 0x2525, 0x2020, 0x1515};
static const uint8_t LCD_volume[10] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

void mp3_init();

void mp3_configured_pin();
void pin_configure_channel_as_io_pin(gpio__port_e port_num, uint8_t pin_num, gpio__function_e func);

void deactive_reset_decoder();
void active_reset_decoder();

void xdcs_decoder_high();
void xdcs_decoder_low();

void ds_decoder();
void cs_decoder();

void set_volume(uint16_t volume);
void volume_helper(uint16_t vols);
void set_bass(uint16_t bass);
void set_treble(uint16_t tre);

void spi_send_to_mp3_decoder(uint8_t data_byte);
void mp3_write(uint8_t address_register, uint16_t high_low_byte_data);