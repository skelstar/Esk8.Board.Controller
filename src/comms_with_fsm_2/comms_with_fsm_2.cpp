#define DEBUG_OUT Serial
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#include <Arduino.h>
#include <VescData.h>
#include <elapsedMillis.h>

#include <RF24Network.h>
#include <NRF24L01Lib.h>

#define SPI_CE 33
#define SPI_CS 26

#define COMMS_BOARD 00
#define COMMS_CONTROLLER  01

//------------------------------------------------------------------

VescData board_packet;

ControllerData controller_packet;

NRF24L01Lib nrf24;

RF24 radio(SPI_CE, SPI_CS);
RF24Network network(radio);

#define NUM_RETRIES 5

elapsedMillis since_sent_to_board;

#include <comms_fsm_2.h>
//------------------------------------------------------------------

void packet_available_cb(uint16_t from_id, uint8_t type)
{

  uint8_t buff[sizeof(VescData)];
  nrf24.read_into(buff, sizeof(VescData));
  memcpy(&board_packet, &buff, sizeof(VescData));

  DEBUGVAL(from_id, board_packet.id, since_sent_to_board);
}

void send_to_board()
{
    DEBUGVAL("sending...", controller_packet.id);

    uint8_t bs[sizeof(ControllerData)];
    memcpy(bs, &controller_packet, sizeof(ControllerData));

    uint8_t retries = nrf24.send_with_retries(/*to*/COMMS_BOARD, /*type*/ 0, bs, sizeof(ControllerData), NUM_RETRIES);
    if (retries > 0)
    {
      DEBUGVAL(retries);
    }
    controller_packet.id++;
}

void setup()
{
  Serial.begin(115200);

  nrf24.begin(&radio, &network, COMMS_CONTROLLER, packet_available_cb);

  add_comms_fsm_transitions();

  DEBUG("Ready to rx from board...");
}

void loop()
{
  if (since_sent_to_board > 1000)
  {
    since_sent_to_board = 0;
    send_to_board();
  }

  nrf24.update();

  comms_fsm.run_machine();
}