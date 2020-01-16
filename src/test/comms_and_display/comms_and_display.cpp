#define DEBUG_OUT Serial
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#include <Arduino.h>
#include <VescData.h>
#include <elapsedMillis.h>

#include <RF24Network.h>
#include <NRF24L01Library.h>
// #include <SSD1306.h>

#ifndef Wire
#include <Wire.h>
#endif

#include <U8g2lib.h>

//------------------------------------------------------------------

#define SPI_CE 33
#define SPI_CS 26

#define OLED_SCL 15
#define OLED_SDA 4
#define OLED_RST 16
#define OLED_ADDR 0x3C
#define OLED_CONTRAST_HIGH 100 // 256 highest
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R2, /* clock=*/OLED_SCL, /* data=*/OLED_SDA, /* reset=*/OLED_RST);

//--------------------------------------------------------------------------------
void lcd_setup()
{
  u8g2.begin();
  u8g2.setContrast(OLED_CONTRAST_HIGH);
  u8g2.clearBuffer();
}

#define FONT_SIZE_MED u8g2_font_profont17_tr

/* 172 ms*/
void lcd_write_text(uint8_t x, uint8_t y, char *text, bool send)
{
  u8g2.setFont(FONT_SIZE_MED); // full
  u8g2.setFontPosTop();
  u8g2.setDrawColor(1);
  u8g2.drawStr(x, y, text);
  if (send)
  {
    u8g2.sendBuffer();
  }
}
//------------------------------------------------------------------

VescData old_vescdata, board_packet;

ControllerData controller_packet, old_packet;

NRF24L01Lib nrf24;

RF24 radio(SPI_CE, SPI_CS);
RF24Network network(radio);

//------------------------------------------------------------------

void nrf_read(uint8_t *data, uint8_t data_len)
{
  nrf24.read_into(data, data_len);
}

void board_packet_available_cb(uint16_t from_id, uint8_t type)
{
  uint8_t buff[sizeof(VescData)];
  nrf_read(buff, sizeof(VescData));
  memcpy(&board_packet, &buff, sizeof(VescData));

  DEBUGVAL(board_packet.id);
}

uint8_t send_with_retries(uint8_t *data, uint8_t data_len, uint8_t num_retries)
{
  uint8_t success, retries = 0;
  do
  {
    success = nrf24.sendPacket(00, /*type*/ 0, data, data_len);
    if (success == false)
    {
      vTaskDelay(1);
    }
    // DEBUGVAL(success, retries);
  } while (!success && retries++ < num_retries);

  return retries;
}
//------------------------------------------------------------

uint8_t retries;

void comms_task_1(void *pvParameters)
{
  elapsedMillis since_sent_to_board = 0;

  nrf24.begin(&radio, &network, 1, board_packet_available_cb);

  while (true)
  {
    if (since_sent_to_board > 150)
    {
      if (since_sent_to_board > 160)
      {
        DEBUGVAL(since_sent_to_board);
      }
      since_sent_to_board = 0;
      controller_packet.id++;
      uint8_t bs[sizeof(ControllerData)];
      memcpy(bs, &controller_packet, sizeof(ControllerData));

      retries += send_with_retries(bs, sizeof(ControllerData), 5);
    }

    nrf24.update();

    vTaskDelay(10);
  }
  vTaskDelete(NULL);
}
//------------------------------------------------------------

void setup()
{
  Serial.begin(115200);

  Serial.printf("Ready...\n");

  lcd_setup();

  xTaskCreatePinnedToCore(comms_task_1, "comms_task_1", 10000, NULL, /*priority*/ 4, NULL, /*core*/ 1);
}

elapsedMillis since_drew_lcd = 0;
bool dot;

void loop()
{
  if (since_drew_lcd > 2000)
  {
    since_drew_lcd = 0;
    char buff[14];
    sprintf(buff, "retries: %d %d", retries, dot);
    dot = !dot;
    lcd_write_text(0, 0, buff, false);
    lcd_write_text(0, 15, buff, false);
    lcd_write_text(0, 30, buff, false);
    lcd_write_text(0, 45, buff, true);
  }
}