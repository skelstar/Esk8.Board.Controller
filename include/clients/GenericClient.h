
#ifndef NRF24L01Lib_h
#include <NRF24L01Lib.h>
#endif

template <typename IN, typename OUT>
class GenericClient
{
public:
  GenericClient(NRF24L01Lib *nrf, uint8_t to)
  {
    _nrf24 = nrf;
    _to = to;
  }

  // template <typename T>
  IN read()
  {
    IN ev;
    uint8_t buff[sizeof(IN)];
    _nrf24->read_into(buff, sizeof(IN));
    memcpy(&ev, &buff, sizeof(IN));
    return ev;
  }

  // template <typename T>
  bool sendTo(uint8_t t, OUT data)
  {
    uint8_t len = sizeof(OUT);
    uint8_t bs[len];
    memcpy(bs, &data, len);
    // takes 3ms if OK, 30ms if not OK
    return _nrf24->send(_to, t, bs, len);
  }

private:
  NRF24L01Lib *_nrf24;
  uint8_t _to;
};