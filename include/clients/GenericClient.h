

typedef void (*PacketAvailableCallback)(uint16_t from, uint8_t type);

bool radioInitialised = false;

template <typename IN, typename OUT>
class GenericClient
{
public:
  GenericClient(uint8_t to)
  {
    _to = to;
  }

  void begin(
      RF24Network *network,
      PacketAvailableCallback packetAvailableCallback,
      Packet::Type packetType)
  {
    _network = network;
    _packetAvailableCallback = packetAvailableCallback;
    _packetType = packetType;
  }

  IN read()
  {
    RF24NetworkHeader header;
    IN ev;
    uint8_t len = sizeof(IN);
    uint8_t buff[len];
    _network->read(header, buff, len);
    memcpy(&ev, &buff, len);
    return ev;
  }

  bool sendTo(uint8_t type, OUT data)
  {
    uint8_t len = sizeof(OUT);
    uint8_t bs[len];
    memcpy(bs, &data, len);
    // takes 3ms if OK, 30ms if not OK
    RF24NetworkHeader header(_to, type);
    _connected = _network->write(header, bs, len);
    return _connected;
  }

  void update()
  {
    _network->update();
    if (_network->available())
    {
      _connected = true;
      RF24NetworkHeader header;
      _network->peek(header);
      if (header.type == _packetType)
        _packetAvailableCallback(header.from_node, header.type);
    }
  }

  bool connected()
  {
    return _connected;
  }

private:
  // RF24 *_radio;
  RF24Network *_network;
  uint8_t _to;
  PacketAvailableCallback _packetAvailableCallback;
  Packet::Type _packetType;
  bool _connected = true;
};