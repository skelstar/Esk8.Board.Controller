#ifdef SERIAL__DEBUG
#define DEBUG_OUT Serial
#endif
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#include <Arduino.h>
#include <elapsedMillis.h>

#include <SPI.h>
#include <RF24Network.h>
#include <NRF24L01Lib.h>
#include <TFT_eSPI.h>

#define NRF_MOSI 23 // blue?
#define NRF_MISO 19 // orange?
#define NRF_CLK 18  // yellow
#define NRF_CS 12 // green
#define NRF_CE 16 // same as tft 15 // white

#define COMMS_BOARD 00
#define COMMS_CONTROLLER 01

//------------------------------------------------------------------

NRF24L01Lib nrf24;

RF24 radio(NRF_CE, NRF_CS);
RF24Network network(radio);

void packet_available_cb(uint16_t from_id, uint8_t type)
{
}

TFT_eSPI tft = TFT_eSPI(135, 240); // Invoke custom library

elapsedMillis since_sent_to_board;

//------------------------------------------------------------

#include <tft.h>

void init_tft()
{
  DEBUG("setup_tft()");
  setup_tft();
}

void init_nrf()
{
  DEBUG("setup_nrf()");
  nrf24.begin(&radio, &network, COMMS_CONTROLLER, packet_available_cb);
}


void setup()
{
  Serial.begin(115200);
  delay(100);

  // init_tft();
  init_nrf();
}

void loop()
{
  vTaskDelay(100);
}
//------------------------------------------------------------------
