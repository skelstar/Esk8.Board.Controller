#define DEBUG_OUT Serial
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#include <Arduino.h>
#include <VescData.h>
#include <elapsedMillis.h>

// used in TFT_eSPI library as alternate SPI port (HSPI?)
#define SOFT_SPI_MOSI_PIN 13 // Blue
#define SOFT_SPI_MISO_PIN 12 // Orange
#define SOFT_SPI_SCK_PIN 15  // Yellow

#define SPI_CE 17
#define SPI_CS 2

#include <RF24Network.h>
#include <NRF24L01Lib.h>

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

void packetAvailable_cb(uint16_t from_id, uint8_t type)
{
  ControllerData board_packet;

  uint8_t buff[sizeof(ControllerData)];
  nrf24.read_into(buff, sizeof(ControllerData));
  memcpy(&board_packet, &buff, sizeof(ControllerData));

  DEBUGVAL(from_id, board_packet.id, since_sent_to_board);
}
//------------------------------------------------------------------

void setup()
{
  Serial.begin(115200);

  nrf24.begin(&radio, &network, COMMS_CONTROLLER, packetAvailable_cb);

  DEBUG("Ready to rx from board...");
}
//------------------------------------------------------------------

void loop()
{
  if (since_sent_to_board > 1000)
  {
    since_sent_to_board = 0;
    DEBUG("sending..");

    uint8_t bs[sizeof(VescData)];
    memcpy(bs, &board_packet, sizeof(VescData));

    uint8_t retries = nrf24.send_with_retries(/*to*/ COMMS_BOARD, /*type*/ 0, bs, sizeof(VescData), NUM_RETRIES);
    if (retries > 0)
    {
      DEBUGVAL(retries);
    }
    board_packet.id++;
  }

  nrf24.update();
}