
void processHUDPacket();
void processBoardPacket();
void sendConfigToBoard();
void sendPacketToBoard();
bool sendPacket(uint8_t *d, uint8_t len, uint8_t packetType);

//------------------------------------------------------------------
void packetAvailable_cb(uint16_t from_id, uint8_t type)
{
  if (type == (uint8_t)PacketType::HUD)
  {
    processHUDPacket();
  }
  else
  {
    processBoardPacket();
  }
  nrfCommsQueueManager->send(EV_COMMS_PKT_RXD);
}
//------------------------------------------------------------------
void processHUDPacket()
{
  sinceLastHudPacket = 0;

  HudActionEvent ev;
  uint8_t buff[sizeof(HudActionEvent)];
  nrf24.read_into(buff, sizeof(HudActionEvent));
  memcpy(&ev, &buff, sizeof(HudActionEvent));

  switch (ev)
  {
  case HUD_ACTION_NONE:
  case HUD_ACTION_HEARTBEAT:
  case HUD_ACTION_DOUBLE_CLICK:
  case HUD_ACTION_TRIPLE_CLICK:
    Serial.printf("<-- HUD: %s\n", hudActionEventNames[(int)ev]);
    break;
  }
}
//------------------------------------------------------------------
void processBoardPacket()
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

    nrfCommsQueueManager->send(EV_COMMS_BD_FIRST_PACKET);

    controller_packet.id = 0;
    sendConfigToBoard();

    sinceBoardConnected = 0;
  }
  else if (board.startedMoving())
  {
    displayChangeQueueManager->send(DISP_EV_MOVING);
    hudMessageQueue->send(HUD_CMD_HEARTBEAT);
  }
  else if (board.hasStopped())
  {
    displayChangeQueueManager->send(DISP_EV_STOPPED);
  }
  else if (board.valuesChanged())
  {
    Serial.printf("board value changed\n");
    displayChangeQueueManager->send(DISP_EV_UPDATE);
  }

  if (board.getCommand() == CommandType::RESET)
  {
    ESP.restart();
  }
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
  if (!rxLastResponse && stats.boardConnected)
  {
    stats.total_failed_sending += 1;
    DEBUGVAL(board.packet.id, controller_packet.id);
  }

  uint8_t bs[sizeof(ControllerData)];
  memcpy(bs, &controller_packet, sizeof(ControllerData));

  vTaskSuspendAll();
  sendPacket(bs, sizeof(ControllerData), PacketType::CONTROL);
  xTaskResumeAll();

  controller_packet.id++;
}
//------------------------------------------------------------------

bool sendPacketToHud(HUDCommand command, bool print = false)
{
  HUDData packet;
  packet.id = hudData.id++;
  packet.state = command;
  elapsedMillis sinceSendingToHud = 0;

  uint8_t bs[sizeof(HUDData)];
  memcpy(bs, &packet, sizeof(HUDData));
  // takes 3ms if OK, 30ms if not OK
  vTaskSuspendAll();
  bool success = nrf24.send(COMMS_HUD, PacketType::HUD, bs, sizeof(HUDData));
  xTaskResumeAll();
  if (print)
    Serial.printf("--> HUD:%s (%d) - %s (took %lums)\n", hudCommandNames[command], packet.id, success ? "SUCCESS" : "FAILED!!!", (unsigned long)sinceSendingToHud);
  return success;
}
//------------------------------------------------------------------

bool sendPacket(uint8_t *d, uint8_t len, uint8_t packetType)
{
  bool sent = nrf24.send(COMMS_BOARD, packetType, d, len);

  return sent;
}
//------------------------------------------------------------------

bool boardTimedOut()
{
  unsigned long timeout = SEND_TO_BOARD_INTERVAL * NUM_MISSED_PACKETS_MEANS_DISCONNECTED;
  return board.sinceLastPacket > (timeout + 100);
}

//------------------------------------------------------------------