
#ifndef NRF24L01Lib_h
#include <NRF24L01Lib.h>
#endif

class HUDClient
{
public:
  HUDClient(NRF24L01Lib *nrf)
  {
    _nrf24 = nrf;
  }

  template <typename T>
  T read()
  {
    T ev;
    uint8_t buff[sizeof(T)];
    _nrf24->read_into(buff, sizeof(T));
    memcpy(&ev, &buff, sizeof(T));
    return ev;
  }

  template <typename T>
  bool sendTo(uint8_t who, uint8_t t, T data)
  {
    uint8_t len = sizeof(T);
    uint8_t bs[len];
    memcpy(bs, &data, len);
    // takes 3ms if OK, 30ms if not OK
    return _nrf24->send(who, t, bs, len);
  }

private:
  NRF24L01Lib *_nrf24;
};