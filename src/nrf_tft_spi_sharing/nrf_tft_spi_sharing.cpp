#define DEBUG_OUT Serial
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#include <Arduino.h>
#include <VescData.h>
#include <elapsedMillis.h>

#include <RF24Network.h>
#include <NRF24L01Lib.h>

#include <TFT_eSPI.h>

// #define SPI_MISO // 19 Orange
// #define SPI_MOSI // 23 Blue
// #define SPI_CLK  // 18 Yellow

#define SPI_CE 26
#define SPI_CS 33

#define COMMS_BOARD 00
#define COMMS_CONTROLLER 01

//------------------------------------------------------------------

VescData board_packet;

NRF24L01Lib nrf24;

RF24 radio(SPI_CE, SPI_CS);
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
  ControllerData board_packet;

  uint8_t buff[sizeof(ControllerData)];
  nrf24.read_into(buff, sizeof(ControllerData));
  memcpy(&board_packet, &buff, sizeof(ControllerData));

  DEBUGVAL(from_id, board_packet.id, since_sent_to_board);
}
//------------------------------------------------------------------

enum SpiDevice
{
  NRF_SPI,
  TFT_SPI
};

void set_SPI_to(SpiDevice device)
{
  if (device == NRF_SPI)
  {
    DEBUG("device == NRF_SPI");
    SPI.setClockDivider(SPI_CLOCK_DIV32);
    SPI.setDataMode(SPI_MODE1);
  }
  else if (device == TFT_SPI)
  {
    DEBUG("device == TFT_SPI");
    SPI.setFrequency(4096);
    SPI.setDataMode(SPI_MODE3);
  }
}
//------------------------------------------------------------------

void init_tft()
{
  set_SPI_to(TFT_SPI);

  tft.init();

  Serial.printf("tft clk_div: %lu\n", SPI.getClockDivider()); // nrf=20713473, tft=4097

  tft.setRotation(1);       // 0 is portrait
  tft.fillScreen(TFT_BLUE); // Clear screen
  tft.setTextColor(TFT_WHITE, TFT_BLUE);
  tft.setTextSize(3);
  tft.drawString("ready", 20, 20);
  pinMode(4, OUTPUT);
  digitalWrite(4, HIGH); // Backlight on

  DEBUG("setup_tft()");
}

void init_nrf()
{
  set_SPI_to(NRF_SPI);
  nrf24.begin(&radio, &network, COMMS_CONTROLLER, packet_available_cb);

  radio.csn(HIGH); // take off SPI bus
}

void setup()
{
  Serial.begin(115200);

  init_nrf();

  delay(200);

  init_tft();

  DEBUG("Ready to rx from board...");
}

int i = 0;

void loop()
{
  if (since_sent_to_board > 1000)
  {
    since_sent_to_board = 0;
    DEBUG("sending..");

    tft.drawNumber(i, 20, 20);
    i++;

    // uint8_t bs[sizeof(VescData)];
    // memcpy(bs, &board_packet, sizeof(VescData));

    // uint8_t retries = nrf24.send_with_retries(/*to*/ COMMS_BOARD, /*type*/ 0, bs, sizeof(VescData), NUM_RETRIES);
    // if (retries > 0)
    // {
    //   DEBUGVAL(retries);
    // }
    // board_packet.id++;
  }

  // nrf24.update();
}