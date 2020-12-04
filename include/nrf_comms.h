
void sendConfigToBoard();
void sendPacketToBoard();
bool sendPacket(uint8_t *d, uint8_t len, uint8_t packetType);
bool sendPacketWithRetries(uint8_t *d, uint8_t len, uint8_t packetType, uint8_t numRetries);

//------------------------------------------------------------------
void packetAvailable_cb(uint16_t from_id, uint8_t type)
{
  sinceLastBoardPacketRx = 0;

  if (from_id == COMMS_HUD)
  {
    Serial.printf("WARNINGL rx from COMMS_HUD. Ignored.\n");
    return;
  }

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
    commsEventQueue->send(CommsState::EV_COMMS_BD_FIRST_PACKET);

    controller_packet.id = 0;
    sendConfigToBoard();

    sinceBoardConnected = 0;
  }
  else if (board.startedMoving())
  {
    displayChangeQueueManager->send(DISP_EV_MOVING);
  }
  else if (board.hasStopped())
  {
    displayChangeQueueManager->send(DISP_EV_STOPPED);
  }
  else if (board.valuesChanged())
  {
    displayChangeQueueManager->send(DISP_EV_UPDATE);
  }

  if (board.getCommand() == CommandType::RESET)
  {
    ESP.restart();
  }

  commsEventQueue->send(CommsState::EV_COMMS_PKT_RXD);
}
//------------------------------------------------------------------

void sendConfigToBoard()
{
  controller_config.id = controller_packet.id;
  uint8_t bs[sizeof(ControllerConfig)];
  memcpy(bs, &controller_config, sizeof(ControllerConfig));
  uint8_t len = sizeof(ControllerConfig);

  sendPacket(bs, len, PacketType::CONFIG);
}
//------------------------------------------------------------------

void sendPacketToBoard()
{
  bool rxLastResponse = board.packet.id == controller_packet.id - 1 &&
                        board.packet.id > 0;
  if (!rxLastResponse && comms_state_connected)
  {
    stats.total_failed_sending += 1;
    DEBUGVAL(board.packet.id, controller_packet.id);
  }

  uint8_t bs[sizeof(ControllerData)];
  memcpy(bs, &controller_packet, sizeof(ControllerData));

  sendPacket(bs, sizeof(ControllerData), PacketType::CONTROL);

  controller_packet.id++;
}
//------------------------------------------------------------------

bool sendPacket(uint8_t *d, uint8_t len, uint8_t packetType)
{
  bool sent = nrf24.send(COMMS_BOARD, packetType, d, len);

  return sent;
}
//------------------------------------------------------------------

bool sendPacketWithRetries(uint8_t *d, uint8_t len, uint8_t packetType, uint8_t maxTries)
{
  bool sent = false;
  int tries = 0;
  while (!sent && tries++ < maxTries)
  {
    sent = nrf24.send(COMMS_BOARD, packetType, d, len);
    if (!sent)
      vTaskDelay(10);
  }
#ifdef PRINT_SEND_RETRIES
  if (tries > 1)
    // DEBUGMVAL("Retried: ", tries, sent);
    DEBUGVAL("Retried: ", tries, sent);
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