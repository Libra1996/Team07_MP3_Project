#include "FreeRTOS.h"
#include "LCD.h"
#include "adc.h"
#include "cli_handlers.h"
#include "delay.h"
#include "ff.h"
#include "gpio.h"
#include "gpio_lab.h"
#include "irs_lab.h"
#include "lpc_peripherals.h"
#include "mp3_init.h"
#include "pwm1.h"
#include "queue.h"
#include "sj2_cli.h"
#include "song_list.h"
#include "spi0.h"
#include "task.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* -------------------------------------------------------------------------- */
/*                           GLOBAL VARIABLE SECTION                          */
/* -------------------------------------------------------------------------- */

typedef char songname_t[32];
char current_playing[32];

bool main_page = true;
bool play_pause = true;
bool setting = false;
bool bass_flag = false;
bool treble_flag = false;
uint16_t cursor_main = 0;

clock_t seconds;

/* -------------------------------------------------------------------------- */
/*                              SEMAPHORE SECTION                             */
/* -------------------------------------------------------------------------- */

SemaphoreHandle_t pause_semaphore;
SemaphoreHandle_t next_song;
SemaphoreHandle_t previous_song;
SemaphoreHandle_t move_down;
SemaphoreHandle_t move_up;

TaskHandle_t play_handle;

/* -------------------------------------------------------------------------- */
/*                                   QUEUE                                    */
/* -------------------------------------------------------------------------- */

QueueHandle_t Q_songname;
QueueHandle_t Q_songdata;
QueueHandle_t adc_to_pwm_task_queue;

/* -------------------------------------------------------------------------- */
/*                             TASK DECLARATION                               */
/* -------------------------------------------------------------------------- */

void mp3_reader_task();
void mp3_player_task();
void adc_task(void *p);
void pwm_task(void *p);

void pause_task();
void next_song_task();
void previous_song_task();
void shuffle_song();
void move_choice_down();
void move_choice_up();

void song_list();

void bass_and_treble();

/* -------------------------------------------------------------------------- */
/*                             INTERUPT SERVICE                               */
/* -------------------------------------------------------------------------- */

// Song list task
uint8_t current_page = 0;
uint8_t current_page_count = 1; // Count the cursor location (1 - 3)
uint8_t cursor_song = 0;        // Song number

bool song_list_flag = false;
bool current_playing_flag = false;

void pause_isr() { xSemaphoreGiveFromISR(pause_semaphore, NULL); }

void next_song_and_move_down_isr() {
  if (song_list_flag == false) {
    xSemaphoreGiveFromISR(next_song, NULL);
  } else {
    xSemaphoreGiveFromISR(move_down, NULL);
  }
}

void prev_song_and_move_up_isr() {
  if (song_list_flag == false) {
    xSemaphoreGiveFromISR(previous_song, NULL);
  } else {
    xSemaphoreGiveFromISR(move_up, NULL);
  }
}

/* -------------------------------------------------------------------------- */
/*                          EXTERNAL FUNCTION SECTION                         */
/* -------------------------------------------------------------------------- */

void turn_off_sj2_4_leds() {
  gpio0__set_high(1, 18);
  gpio0__set_high(1, 24);
  gpio0__set_high(1, 26);
  gpio0__set_high(2, 3);
}

bool mp3_decoder_needs_data() {
  return gpio0__get_level(2, 0); // DREQ to PWM1
}

/* -------------------------------------------------------------------------- */
/*                                    MAIN                                    */
/* -------------------------------------------------------------------------- */

void main(void) {
  /* ----------------------------- initialization ----------------------------- */
  sj2_cli__init();
  turn_off_sj2_4_leds();
  song_list__populate();
  LPC_IOCON->P0_10 |= (1 << 10);
  LPC_IOCON->P0_11 |= (1 << 10);
  gpio__construct_with_function(0, 10, 2);
  gpio__construct_with_function(0, 11, 2);
  gpio0__set_as_input(1, 14); // Song list
  gpio0__set_as_input(0, 7);  // Song list PLAY
  mp3_init();
  LCD_init(0x4E, 20, 4);
  load_all_custom();
  LCD_home_menu();

  /* --------------------------- Queue and Semaphore -------------------------- */
  Q_songname = xQueueCreate(1, sizeof(songname_t));
  Q_songdata = xQueueCreate(1, 512);
  adc_to_pwm_task_queue = xQueueCreate(1, sizeof(int));

  pause_semaphore = xSemaphoreCreateBinary();
  next_song = xSemaphoreCreateBinary();
  previous_song = xSemaphoreCreateBinary();
  move_down = xSemaphoreCreateBinary();
  move_up = xSemaphoreCreateBinary();

  /* -------------------------------- Interrupt ------------------------------- */

  lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__GPIO, gpio0__interrupt_dispatcher, "isr for port 0");
  gpio0__attach_interrupt(6, GPIO_INTR__FALLING_EDGE, pause_isr);
  gpio0__attach_interrupt(9, GPIO_INTR__FALLING_EDGE, next_song_and_move_down_isr);
  gpio0__attach_interrupt(25, GPIO_INTR__FALLING_EDGE, prev_song_and_move_up_isr);

  /* ------------------------------ Task creation ----------------------------- */

  xTaskCreate(mp3_reader_task, "reader", (1024 * 2) / sizeof(void *), NULL, PRIORITY_MEDIUM, NULL);
  xTaskCreate(mp3_player_task, "player", (1024 * 2) / sizeof(void *), NULL, PRIORITY_MEDIUM, &play_handle);

  xTaskCreate(adc_task, "vol_1", (1024) / sizeof(void *), NULL, PRIORITY_MEDIUM, NULL);
  xTaskCreate(pwm_task, "vol_2", (1024) / sizeof(void *), NULL, PRIORITY_MEDIUM, NULL);

  xTaskCreate(song_list, "Song_list", (1024) / sizeof(void *), NULL, PRIORITY_MEDIUM, NULL);
  xTaskCreate(move_choice_down, "move down", (1024) / sizeof(void *), NULL, PRIORITY_MEDIUM, NULL);
  xTaskCreate(move_choice_up, "move up", (1024) / sizeof(void *), NULL, PRIORITY_MEDIUM, NULL);

  xTaskCreate(pause_task, "pause", (1024) / sizeof(void *), NULL, PRIORITY_MEDIUM, NULL);
  xTaskCreate(previous_song_task, "previous_song", (1024) / sizeof(void *), NULL, PRIORITY_MEDIUM, NULL);
  xTaskCreate(next_song_task, "next_song", (1024) / sizeof(void *), NULL, PRIORITY_MEDIUM, NULL);

  xTaskCreate(bass_and_treble, "Bass_Treble_setting", (1024) / sizeof(void *), NULL, PRIORITY_MEDIUM, NULL);

  vTaskStartScheduler();
}

/* -------------------------------------------------------------------------- */
/*                              TASK DEFINATION                               */
/* -------------------------------------------------------------------------- */

// Reader tasks receives song-name over Q_songname to start reading it
void mp3_reader_task() {
  songname_t name;
  char bytes_512[512];
  UINT br;

  while (1) {
    xQueueReceive(Q_songname, name, portMAX_DELAY);
    strncpy(current_playing, name, 32);
    LCD_play(current_playing);
    LCD_play_pause(true);

    const char *filename = name;
    FIL file;
    FRESULT result = f_open(&file, filename, FA_READ);

    if (FR_OK == result) {
      f_read(&file, bytes_512, sizeof(bytes_512), &br);
      while (br != 0) {
        f_read(&file, bytes_512, sizeof(bytes_512), &br);
        xQueueSend(Q_songdata, &bytes_512, portMAX_DELAY);
        if (uxQueueMessagesWaiting(Q_songname)) {
          break;
        }
      }

      if (br == 0 && current_playing_flag == true) {
        play_pause = false;
        LCD_play_pause(play_pause);
      }

      f_close(&file);
    } else {
      fprintf(stderr, "Failed to open the file");
    }
  }
}

// Player task receives song data over Q_songdata to send it to the MP3 decoder
void mp3_player_task() {
  char bytes_512[512];

  while (1) {
    xQueueReceive(Q_songdata, bytes_512, portMAX_DELAY);
    for (int i = 0; i < sizeof(bytes_512); i++) {
      while (!mp3_decoder_needs_data()) {
        vTaskDelay(1);
      }
      spi_send_to_mp3_decoder(bytes_512[i]);
    }
  }
}

// Volume task
void adc_task(void *p) {
  adc__initialize();

  adc__enable_burst_mode();

  pin_configure_channel_as_io_pin(GPIO__PORT_1, 31, GPIO__FUNCTION_3);

  int adc_reading = 0;
  while (1) {
    adc_reading = adc__get_channel_reading_with_burst_mode(ADC__CHANNEL_5);
    xQueueSend(adc_to_pwm_task_queue, &adc_reading, 0);
    vTaskDelay(1000);
  }
}

void pwm_task(void *p) {
  int adc_reading = 0;
  uint16_t temp = 0;
  uint16_t t2 = 0;
  uint16_t t3 = 0;
  uint16_t vols = 0;
  uint16_t ba = 0;
  uint16_t tre = 0;

  while (1) {
    if (xQueueReceive(adc_to_pwm_task_queue, &adc_reading, 100)) {
      if (bass_flag) {
        ba = ((adc_reading * 6) / 4095);
        if (t2 == 0) {
          t2 = ba;
        } else if (t2 != ba) {
          clear();
          setCursor(1, 1);
          helper("Bass: ", 0);
          send(bass_treble[ba], 0x01);
          set_bass(ba);
          delay__ms(2000);
          LCD_play(current_playing);
          LCD_play_pause(play_pause);
          t2 = ba;
          temp = ((adc_reading * 10) / 4095);
          t3 = ba;
          bass_flag = false;
          current_playing_flag = true;
        }
      }

      if (treble_flag) {
        tre = ((adc_reading * 6) / 4095);
        if (t3 == 0) {
          t3 = tre;
        } else if (t3 != tre) {
          clear();
          setCursor(1, 1);
          helper("Treble: ", 0);
          send(bass_treble[tre], 0x01);
          set_treble(tre);
          delay__ms(2000);
          LCD_play(current_playing);
          LCD_play_pause(play_pause);
          t3 = tre;
          temp = ((adc_reading * 10) / 4095);
          t2 = tre;
          treble_flag = false;
          current_playing_flag = true;
        }
      }

      if (current_playing_flag == true && (bass_flag == false || treble_flag == false)) {
        vols = ((adc_reading * 10) / 4095);
        if (temp == 0) {
          temp = vols;
        } else if (temp != vols) {
          clear();
          setCursor(1, 1);
          helper("Volume: ", 0);
          send(LCD_volume[vols], 0x01);
          set_volume(vols);
          delay__ms(2000);
          LCD_play(current_playing);
          LCD_play_pause(play_pause);
          temp = vols;
          t2 = ((adc_reading * 6) / 4095);
          t3 = ((adc_reading * 6) / 4095);
        }
      }

      vTaskDelay(250);
    }
  }
}

// Song list task
void song_list() {
  while (1) {
    if (gpio0__get_level(1, 14)) {
      // If in song list => main page
      if (song_list_flag == true && main_page == false && cursor_main == 0) {
        LCD_home_menu();
        song_list_flag = false;
        current_playing_flag = true;
        main_page = true;
        cursor_main = 1;
      }
      // If in main page or current playing page => song list
      else if (main_page == true || current_playing_flag == true) {
        main_page = false;
        song_list_flag = true;
        current_playing_flag = false;
        LCD_song_list(current_page, current_page + 2);
        setCursor(0, current_page_count);
        send(0, 0x01); // >
      }
      // If in song list but there is song playing => current palying
      else if (song_list_flag == true && current_playing_flag == false && cursor_song >= 0 &&
               cursor_song < song_list__get_item_count()) {
        LCD_play(current_playing);
        LCD_play_pause(play_pause);
        song_list_flag = false;
        current_playing_flag = true;
      }
    } else if (gpio0__get_level(0, 7)) {
      if (song_list_flag == true && current_playing_flag == false) {
        const char *song = song_list__get_name_for_item(cursor_song);
        xQueueSend(Q_songname, song, portMAX_DELAY);
        song_list_flag = false;
        current_playing_flag = true;
      }

      vTaskDelay(250);
    }

    vTaskDelay(250);
  }
}

void move_choice_down() {
  uint8_t total_song = song_list__get_item_count() - 1;
  while (1) {
    if (xSemaphoreTake(move_down, portMAX_DELAY)) {
      if (current_page_count != 3 && cursor_song != total_song) {
        // Current page cursor DOWN update
        setCursor(0, current_page_count);
        send(' ', 0x01);
        current_page_count++;
        cursor_song++;
        if (cursor_song != total_song) {
          setCursor(0, current_page_count);
          send(0, 0x01);
        }
      } else if (current_page_count == 3 && cursor_song != total_song) {
        // Fowarding to new page
        cursor_song++;
        current_page_count = 1;
        current_page += 3;
        LCD_song_list(current_page, current_page + 2);
        setCursor(0, current_page_count);
        send(0, 0x01);
      } else if (cursor_song == total_song) {
        // Loop back song list downward
        cursor_song = 0;
        current_page = 0;
        current_page_count = 1;
        LCD_song_list(current_page, current_page + 2);
        setCursor(0, current_page_count);
        send(0, 0x01);
      }

      vTaskDelay(250);
    }
  }
}

void move_choice_up() {
  uint8_t total_song = song_list__get_item_count() - 1;
  while (1) {
    if (xSemaphoreTake(move_up, portMAX_DELAY)) {
      if (current_page_count != 1 && cursor_song != 0) {
        // Current page cursor UP update
        setCursor(0, current_page_count);
        send(' ', 0x01);
        current_page_count--;
        cursor_song--;
        setCursor(0, current_page_count);
        send(0, 0x01);
      } else if (current_page_count == 1 && cursor_song != 0) {
        // Backwarding to new page
        if (cursor_song <= 3) {
          current_page = 0;
          current_page_count = cursor_song;
        } else {
          current_page -= 3;
          current_page_count = 3;
        }
        cursor_song--;
        LCD_song_list(current_page, current_page + 2);
        setCursor(0, current_page_count);
        send(0, 0x01);
      } else if (cursor_song == 0) {
        // Loop back song list upward
        cursor_song = total_song;
        current_page = total_song - 2;
        current_page_count = 3;
        LCD_song_list(current_page, current_page + 2);
        setCursor(0, current_page_count);
        send(0, 0x01);
      }

      vTaskDelay(250);
    }
  }
}

// Pause task
void pause_task() {
  while (1) {
    if (xSemaphoreTake(pause_semaphore, portMAX_DELAY)) {
      if (play_pause == true) {
        vTaskSuspend(play_handle);
        play_pause = false;
      } else {
        vTaskResume(play_handle);
        play_pause = true;
      }

      if (current_playing_flag) {
        LCD_play_pause(play_pause);
      }
    }
  }
}

// Next and Previous song task
void next_song_task() {
  while (1) {
    if (xSemaphoreTake(next_song, portMAX_DELAY) && current_playing_flag == true) {
      vTaskResume(play_handle);

      int total = song_list__get_item_count();
      if (cursor_song == total) {
        cursor_song = 0;
      }
      cursor_song++;
      const char *song = song_list__get_name_for_item(cursor_song);
      xQueueSend(Q_songname, song, portMAX_DELAY);

      vTaskDelay(100);
    }
  }
}

void previous_song_task() {
  while (1) {
    if (xSemaphoreTake(previous_song, portMAX_DELAY) && current_playing_flag == true) {
      vTaskResume(play_handle);

      int total = song_list__get_item_count();
      if (cursor_song == 0) {
        cursor_song = total;
      }
      cursor_song--;
      const char *song = song_list__get_name_for_item(cursor_song);
      xQueueSend(Q_songname, song, portMAX_DELAY);

      vTaskDelay(100);
    }
  }
}

// Bass and Treble setting task
void bass_and_treble() {
  while (1) {
    if (gpio0__get_level(0, 7)) {
      if (current_playing_flag) {
        LCD_bass_treble();
        setCursor(1, 2);
        send(0, 0x01);
        current_playing_flag = false;
        setting = true;
        bass_flag = true;
      } else if (setting && bass_flag) {
        setCursor(1, 2);
        send(' ', 0x01);
        treble_flag = true;
        bass_flag = false;
        bass_treble_cursor(false);
      } else if (setting && treble_flag) {
        setCursor(10, 2);
        send(' ', 0x01);
        treble_flag = false;
        bass_flag = true;
        bass_treble_cursor(true);
      }
    }

    vTaskDelay(250);
  }
}