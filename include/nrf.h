#include <RF24Network.h>
#include <NRF24L01Library.h>

#define SPI_CE  33
#define SPI_CS  26

#define NRF_ADDRESS_SERVER    0
#define NRF_ADDRESS_CLIENT    1

RF24 radio(SPI_CE, SPI_CS);
RF24Network network(radio);

NRF24L01Lib nrf24;

uint16_t board_id;

bool nrf_setup()
{
  nrf24.begin(&radio, &network, NRF_ADDRESS_CLIENT, board_packet_available_cb);
  return true;
}

void nrf_update()
{
  nrf24.update();
}

void nrf_read(uint8_t *data, uint8_t data_len)
{
  nrf24.read_into(data, data_len);
}

bool nrf_send_to_board()
{
  uint8_t bs[sizeof(ControllerData)];
  memcpy(bs, &controller_packet, sizeof(ControllerData));
  return nrf24.sendPacket(board_id, /*type*/0, bs, sizeof(ControllerData));
}

bool send_controller_packet_to_board()
{
  uint8_t bs[sizeof(ControllerData)];
  memcpy(bs, &controller_packet, sizeof(ControllerData));
  
  controller_packet.command = 0;

  bool success = nrf24.sendPacket(board_id, /*type*/ 0, bs, sizeof(ControllerData));

  return success;
}