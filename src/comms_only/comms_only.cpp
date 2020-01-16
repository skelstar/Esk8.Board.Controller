#define DEBUG_OUT Serial
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#include <Arduino.h>
#include <VescData.h>
#include <elapsedMillis.h>

#include <Wire.h>
#include <RF24Network.h>
#include <NRF24L01Library.h>

#define SPI_CE 33
#define SPI_CS 26

//------------------------------------------------------------------

VescData old_vescdata, board_packet;

ControllerData controller_packet, old_packet;

NRF24L01Lib nrf24;

RF24 radio(SPI_CE, SPI_CS);
RF24Network network(radio);

//------------------------------------------------------------------

void setup()
{

  Serial.begin(115200);

  Wire.begin();

  // setupLCD();

  nrf24.begin(&radio, &network, 1, board_packet_available_cb);

  Serial.printf("Ready...");
}

elapsedMillis since_sent_to_board = 0;

void loop()
{
  if (since_sent_to_board > 100)
  {
    since_sent_to_board = 0;
    controller_packet.id++;
    uint8_t bs[sizeof(ControllerData)];
    memcpy(bs, &controller_packet, sizeof(ControllerData));

    uint8_t retries = send_with_retries(bs, sizeof(ControllerData), 5);

    if (retries > 0)
    {
      DEBUGVAL(retries);
    }
  }

  nrf24.update();
}