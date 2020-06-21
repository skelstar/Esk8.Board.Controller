
bool sendConfigToBoard();
void sendPacketToBoard();

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

bool sendConfigToBoard()
{
  sendToCommsEventStateQueue(EV_COMMS_PKT_RXD);

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
    sendToCommsEventStateQueue(EV_COMMS_FAILED_SEND_TO_BOARD);
  }
  // controller_packet.id++;
  return success;
}
//------------------------------------------------------------------

void sendPacketToBoard()
{
  uint8_t bs[sizeof(ControllerData)];
  memcpy(bs, &controller_packet, sizeof(ControllerData));

  bool sent = false;
  int sendRetries = 0;
  while (!sent)
  {
    sent = !nrf24.send_packet(/*to*/ COMMS_BOARD, PacketType::CONTROL, bs, sizeof(ControllerData));
    if (!sent && sendRetries > 3)
    {
      sendToCommsEventStateQueue(EV_COMMS_FAILED_SEND_TO_BOARD);
    }
  }

  // bool success = nrf24.send_packet(/*to*/ COMMS_BOARD, PacketType::CONTROL, bs, sizeof(ControllerData));
  // if (success)
  // {
  //   sendToCommsEventStateQueue(EV_COMMS_PKT_RXD);
  // }
  // else
  // {
  //   sendToCommsEventStateQueue(EV_COMMS_FAILED_SEND_TO_BOARD);
  // }

  controller_packet.id++;
}

//------------------------------------------------------------------
bool boardTimedOut()
{
  unsigned long timeout = SEND_TO_BOARD_INTERVAL * NUM_MISSED_PACKETS_MEANS_DISCONNECTED;
  return board.sinceLastPacket > (timeout + 100);
}

//------------------------------------------------------------------