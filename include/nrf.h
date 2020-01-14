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

uint8_t send_with_retries(uint8_t *data, uint8_t data_len, uint8_t num_retries)
{
  uint8_t success, retries = 0;
  do {
    success = nrf24.sendPacket(board_id, /*type*/ 0, data, data_len);
    if (success == false)
    {
      vTaskDelay(1);
    }
  } while (!success && retries++ < num_retries);

  return retries;
}

bool send_controller_packet_to_board()
{
  uint8_t bs[sizeof(ControllerData)];
  memcpy(bs, &controller_packet, sizeof(ControllerData));
  
  controller_packet.command = 0;

  uint8_t retries = send_with_retries(bs, sizeof(ControllerData), /*num_retries*/ 4);

  bool success = retries > 4;

#ifdef LOG_RETRIES  

  log_retry(retries > 0);

  uint8_t sum_retries = get_sum_retries();

  if (retries > 0 || controller_packet.id % 20 == 0)
  {
    float retry_rate = get_retry_rate(sum_retries, controller_packet.id);
    DEBUGVAL(sum_retries, retry_rate, success, controller_packet.id);
  }
#endif

  return success;
}