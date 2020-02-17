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

#define TFT_SPI_FREQUENCY 40000000
#define TFT_SPI_MODE SPI_MODE3

void set_SPI_to(SpiDevice device)
{
  if (device == NRF_SPI)
  {
    SPI.setFrequency(TFT_SPI_FREQUENCY);
    SPI.setDataMode(TFT_SPI_MODE);
  }
  else if (device == TFT_SPI)
  {
    _SPI.setClockDivider(SPI_CLOCK_DIV32);
  }
}
//------------------------------------------------------------------

void setup()
{
  Serial.begin(115200);

  //set_SPI_to(NRF_SPI);
  nrf24.begin(&radio, &network, COMMS_CONTROLLER, packet_available_cb);

  DEBUG("Ready to rx from board...");
}

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