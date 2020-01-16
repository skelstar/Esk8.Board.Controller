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