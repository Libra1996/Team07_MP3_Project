#include "mp3_init.h"

void mp3_configured_pin() {
  /* -------------------------------------------------------------------------- */
  /*                        INITALIZE PIN FOR SPI DECODER                       */
  /* -------------------------------------------------------------------------- */

  // sck: p0.15
  gpio__construct_with_function(GPIO__PORT_0, 15, GPIO__FUNCTION_2);

  // miso: p0.17
  gpio__construct_with_function(GPIO__PORT_0, 17, GPIO__FUNCTION_2);

  // mosi: p0.18
  gpio__construct_with_function(GPIO__PORT_0, 18, GPIO__FUNCTION_2);

  /* -------------------------------------------------------------------------- */
  /*                             INITIALIZE GPIO PIN                            */
  /* -------------------------------------------------------------------------- */

  // setup DREQ p2.0
  gpio__construct_with_function(GPIO__PORT_2, 0, GPIO__FUNCITON_0_IO_PIN);
  gpio0__set_as_input(2, 0);

  // setup CS (low) p2.1
  gpio__construct_with_function(GPIO__PORT_2, 1, GPIO__FUNCITON_0_IO_PIN);
  gpio0__set_as_output(2, 1);

  // setup XDCS (low) p2.2
  gpio__construct_with_function(GPIO__PORT_2, 2, GPIO__FUNCITON_0_IO_PIN);
  gpio0__set_as_output(2, 2);

  // setup Reset (low) p2.4
  gpio__construct_with_function(GPIO__PORT_2, 4, GPIO__FUNCITON_0_IO_PIN);
  gpio0__set_as_output(2, 4);
}

void pin_configure_channel_as_io_pin(gpio__port_e port_num, uint8_t pin_num, gpio__function_e func) {
  if (func == GPIO__FUNCTION_3) {
    LPC_IOCON->P1_31 &= ~(1 << 7);
  }

  gpio_s tempt = gpio__construct_with_function(port_num, pin_num, func);
}

/* -------------------------------------------------------------------------- */
/*                               CONTROL SIGNAL                               */
/* -------------------------------------------------------------------------- */

void active_reset_decoder() { gpio0__set_low(2, 4); }
void deactive_reset_decoder() { gpio0__set_high(2, 4); }

void xdcs_decoder_high() { gpio0__set_high(2, 2); }
void xdcs_decoder_low() { gpio0__set_low(2, 2); }

void cs_decoder() { gpio0__set_low(2, 1); }
void ds_decoder() { gpio0__set_high(2, 1); }

/* -------------------------------------------------------------------------- */
/*                                DATA ACTION                                 */
/* -------------------------------------------------------------------------- */

void set_volume(uint16_t volume) { mp3_write(SCI_VOL, VOLUME[volume]); }

void set_bass(uint16_t bass) { mp3_write(SCI_BASS_TREBLE, BASS[bass]); }

void set_treble(uint16_t tre) { mp3_write(SCI_BASS_TREBLE, TREBLE[tre]); }

void spi_send_to_mp3_decoder(uint8_t data_byte) {
  while (!gpio0__get_level(2, 0)) {
    ; // waiting for DREQ
  }
  gpio0__set_low(2, 2); // set xdcs low
  ssp0__exchange_byte_lab(data_byte);
  gpio0__set_high(2, 2); // set xdcs high;
}

void mp3_write(uint8_t address_register, uint16_t high_low_byte_data) {
  while (!gpio0__get_level(2, 0)) {
    ; // waiting for DREQ
  }
  cs_decoder();
  // xdcs_decoder_low();
  {
    ssp0__exchange_byte_lab(0x02); // opcode for write operation
    ssp0__exchange_byte_lab(address_register);
    decoder_flash_send_data(high_low_byte_data);
    while (!gpio0__get_level(2, 0)) {
      ; // waiting for DREQ
    }
  }
  // xdcs_decoder_high();
  ds_decoder();
}

/* -------------------------------------------------------------------------- */
/*                            MP3 INITIALIZATION                              */
/* -------------------------------------------------------------------------- */

void mp3_init() {
  mp3_configured_pin();
  // deactive_reset_decoder();
  ssp0__init_mp3(3 * 1000 * 1000);
  ssp0__exchange_byte_lab(0xFF);
  ds_decoder();
  xdcs_decoder_high();
  // set_volume(0x2020);

  mp3_write(SCI_CLOCKF, 0x6000);
}