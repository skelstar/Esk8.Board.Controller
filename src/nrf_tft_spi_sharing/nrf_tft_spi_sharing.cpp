#define DEBUG_OUT Serial
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#include <Arduino.h>
#include <VescData.h>
#include <elapsedMillis.h>

#define SOFT_SPI_MOSI_PIN 13 // Blue
#define SOFT_SPI_MISO_PIN 12 // Orange
#define SOFT_SPI_SCK_PIN 15  // Yellow

#define NRF_CE 26
#define NRF_CS 33

#include <RF24.h>
#include <RF24Network.h>
#include <NRF24L01Lib.h>

#include <TFT_eSPI.h>

#define COMMS_BOARD 00
#define COMMS_CONTROLLER 01

//------------------------------------------------------------------

VescData board_packet;
ControllerData controller_packet;

NRF24L01Lib nrf24;

RF24 radio = RF24(NRF_CE, NRF_CS);
RF24Network network(radio);

#define NUM_RETRIES 5

elapsedMillis since_sent_to_board;

//------------------------------------------------------------------

#define LCD_WIDTH 240
#define LCD_HEIGHT 135

TFT_eSPI tft = TFT_eSPI(LCD_HEIGHT, LCD_WIDTH); // Invoke custom library

//------------------------------------------------------------------

void packet_available_cb(uint16_t from_id, uint8_t type)
{
  uint8_t buff[sizeof(VescData)];
  nrf24.read_into(buff, sizeof(VescData));
  memcpy(&board_packet, &buff, sizeof(VescData));

  // DEBUGVAL(from_id, board_packet.id, since_sent_to_board);
}
//------------------------------------------------------------------

void init_tft()
{
  tft.init();
  tft.setRotation(1);       // 0 is portrait
  tft.fillScreen(TFT_BLUE); // Clear screen
  tft.setTextColor(TFT_WHITE, TFT_BLUE);
  tft.setTextSize(3);
  tft.drawString("ready", 20, 20);

  DEBUG("setup_tft()");
}

void init_nrf()
{
  nrf24.begin(&radio, &network, COMMS_CONTROLLER, packet_available_cb);
}

void display_task_0(void *pvParameters)
{
  elapsedMillis since_updated_display;

  int j = 0;

  init_tft();

  Serial.printf("display_task_0 running on core %d\n", xPortGetCoreID());

  while (true)
  {
    if (since_updated_display > 10)
    {
      since_updated_display = 0;
      tft.drawNumber((unsigned long)board_packet.id, 20, 20);
    }

    vTaskDelay(5);
  }
  vTaskDelete(NULL);
}

void setup()
{
  Serial.begin(115200);

  xTaskCreatePinnedToCore(display_task_0, "display_task_0", 10000, NULL, /*priority*/ 3, NULL, /*core*/ 0);

  delay(1000);
  init_nrf();
}

int i = 0;

void loop()
{
  if (since_sent_to_board > 200)
  {
    since_sent_to_board = 0;

    i++;

    uint8_t bs[sizeof(ControllerData)];
    memcpy(bs, &controller_packet, sizeof(ControllerData));

    uint8_t retries = nrf24.send_with_retries(/*to*/ COMMS_BOARD, /*type*/ 0, bs, sizeof(ControllerData), NUM_RETRIES);
    if (retries > 0)
    {
      DEBUGVAL(retries, since_sent_to_board);
    }
    controller_packet.id++;
  }
  // delay(1000);
  nrf24.update();
}