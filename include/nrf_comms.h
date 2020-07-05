
void sendConfigToBoard();
void sendPacketToBoard();
bool sendPacket(uint8_t *d, uint8_t len, uint8_t packetType);
bool sendPacketWithRetries(uint8_t *d, uint8_t len, uint8_t packetType, uint8_t numRetries);

//------------------------------------------------------------------
void packetAvailable_cb(uint16_t from_id, uint8_t type)
{
  sinceLastBoardPacketRx = 0;

  VescData board_packet;
  uint8_t buff[sizeof(VescData)];
  nrf24.read_into(buff, sizeof(VescData));
  memcpy(&board_packet, &buff, sizeof(VescData));

  board.save(board_packet);

  if (board.packet.reason == FIRST_PACKET)
  {
    /*
    * send board reset event to commsState
    * set controller_packet.id = 0
    * send controller "CONFIG" packet to board
    */
    DEBUG("*** board's first packet!! ***");
    sendToCommsEventStateQueue(EV_COMMS_BD_RESET);

    controller_packet.id = 0;
    sendConfigToBoard();
  }
  else if (board.startedMoving())
  {
    send_to_display_event_queue(DISP_EV_MOVING);
  }
  else if (board.hasStopped())
  {
    send_to_display_event_queue(DISP_EV_STOPPED);
  }
  else if (board.valuesChanged())
  {
    send_to_display_event_queue(DISP_EV_UPDATE);
  }

  sendToCommsEventStateQueue(EV_COMMS_PKT_RXD);
}
//------------------------------------------------------------------

void sendConfigToBoard()
{
  controller_config.id = controller_packet.id;
  uint8_t bs[sizeof(ControllerConfig)];
  memcpy(bs, &controller_config, sizeof(ControllerConfig));
  uint8_t len = sizeof(ControllerConfig);

  sendPacket(bs, len, PacketType::CONFIG);

  // if (!sendPacketWithRetries(bs, len, PacketType::CONFIG, MAX_SEND_ATTEMPTS))
  // {
  //   sendToCommsEventStateQueue(EV_COMMS_FAILED_SEND_TO_BOARD);
  // }

  // controller_packet.id++;
}
//------------------------------------------------------------------

void sendPacketToBoard()
{
  uint8_t bs[sizeof(ControllerData)];
  memcpy(bs, &controller_packet, sizeof(ControllerData));
  uint8_t len = sizeof(ControllerData);

  sendPacket(bs, sizeof(ControllerData), PacketType::CONTROL);

  // if (!sendPacketWithRetries(bs, sizeof(ControllerData), PacketType::CONTROL, MAX_SEND_ATTEMPTS))
  // {
  //   sendToCommsEventStateQueue(EV_COMMS_FAILED_SEND_TO_BOARD);
  // }

  controller_packet.id++;
}
//------------------------------------------------------------------

bool sendPacket(uint8_t *d, uint8_t len, uint8_t packetType)
{
  bool sent = nrf24.send_packet(COMMS_BOARD, packetType, d, len);

  return sent;
}
//------------------------------------------------------------------

bool sendPacketWithRetries(uint8_t *d, uint8_t len, uint8_t packetType, uint8_t maxTries)
{
  bool sent = false;
  int tries = 0;
  while (!sent && tries++ < maxTries)
  {
    sent = nrf24.send_packet(COMMS_BOARD, packetType, d, len);
    if (!sent)
      vTaskDelay(10);
  }
#ifdef PRINT_SEND_RETRIES
  if (tries > 1)
    DEBUGMVAL("Retried: ", tries, sent);
#endif

  return sent;
}
//------------------------------------------------------------------

bool boardTimedOut()
{
  unsigned long timeout = SEND_TO_BOARD_INTERVAL * NUM_MISSED_PACKETS_MEANS_DISCONNECTED;
  return board.sinceLastPacket > (timeout + 100);
}

//------------------------------------------------------------------