#include <RF24Network.h>
#include <NRF24L01Library.h>

#define SPI_CE  33
#define SPI_CS  26

RF24 radio(SPI_CE, SPI_CS);
RF24Network network(radio);

NRF24L01Lib nrf24;

uint16_t board_id;

bool nrf_setup()
{
  nrf24.begin(&radio, &network, nrf24.RF24_CLIENT, packet_available_cb);
  return true;
}