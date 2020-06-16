
bool sendConfigToBoard();
bool sendPacketToBoard(PacketType packetType);

//------------------------------------------------------------------
void packetAvailable_cb(uint16_t from_id, uint8_t type)
{
  VescData board_packet;

  sinceLastBoardPacketRx = 0;

  uint8_t buff[sizeof(VescData)];
  nrf24.read_into(buff, sizeof(VescData));
  memcpy(&board_packet, &buff, sizeof(VescData));

  board.save(board_packet);

  if (board.packet.reason == FIRST_PACKET)
  {
    /*
    * set controller_packet.id = 0
    * send board reset event to commsState
    * send controller "CONFIG" packet to board
    */
    DEBUG("*** board's first packet!! ***");
    controller_packet.id = 0;
    sendToCommsEventStateQueue(EV_COMMS_BD_RESET);

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

bool sendConfigToBoard()
{
  controller_config.id = controller_packet.id;
  uint8_t bs[sizeof(ControllerConfig)];
  memcpy(bs, &controller_config, sizeof(ControllerConfig));

  bool success = nrf24.send_packet(/*to*/ COMMS_BOARD, PacketType::CONFIG, bs, sizeof(ControllerConfig));
  if (success)
  {
    sendToCommsEventStateQueue(EV_COMMS_PKT_RXD);
  }
  else
  {
    sendToCommsEventStateQueue(EV_COMMS_BOARD_TIMEDOUT);
  }
  // controller_packet.id++;
  return success;
}
//------------------------------------------------------------------

bool sendPacketToBoard()
{
  uint8_t bs[sizeof(ControllerData)];
  memcpy(bs, &controller_packet, sizeof(ControllerData));

  bool success = nrf24.send_packet(/*to*/ COMMS_BOARD, PacketType::CONTROL, bs, sizeof(ControllerData));
  if (success)
  {
    sendToCommsEventStateQueue(EV_COMMS_PKT_RXD);
  }
  else
  {
    sendToCommsEventStateQueue(EV_COMMS_BOARD_TIMEDOUT);
  }
  controller_packet.id++;
  return success;
}

//------------------------------------------------------------------
bool boardTimedOut()
{
  unsigned long timeout = SEND_TO_BOARD_INTERVAL * NUM_MISSED_PACKETS_MEANS_DISCONNECTED;
  return board.sinceLastPacket > (timeout + 100);
}