
bool sendConfigToBoard();
bool sendPacketToBoard(PacketType packetType);
void manageResponses(bool success);
bool vescValuesChanged(VescData oldVals, VescData newVals);

//------------------------------------------------------------------
void packetAvailable_cb(uint16_t from_id, uint8_t type)
{
  sinceLastBoardPacketRx = 0;

  old_board_packet = board_packet;

  uint8_t buff[sizeof(VescData)];
  nrf24.read_into(buff, sizeof(VescData));
  memcpy(&board_packet, &buff, sizeof(VescData));

  if (board_packet.reason == FIRST_PACKET)
  {
    /*
    - set controller_packet.id = 0
    - send board reset event to commsState
    - send CONFIG packet to board
    */
    DEBUG("*** board's first packet!! ***");
    controller_packet.id = 0;
    sendToCommsEventStateQueue(EV_COMMS_BD_RESET);

    bool success = sendConfigToBoard();
  }
  else if (old_board_packet.moving != board_packet.moving)
  {
    // board is moving
    send_to_display_event_queue(board_packet.moving ? DISP_EV_MOVING : DISP_EV_STOPPED);
  }
  else if (vescValuesChanged(old_board_packet, board_packet))
  {
    // important values have changed
    send_to_display_event_queue(DISP_EV_UPDATE);
  }
  else
  {
    sendToCommsEventStateQueue(EV_COMMS_PKT_RXD);
  }
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
void manageResponses(bool success)
{
  if (success)
  {
    sendToCommsEventStateQueue(EV_COMMS_PKT_RXD);
  }
  else
  {
    sendToCommsEventStateQueue(EV_COMMS_BOARD_TIMEDOUT);
  }
}

//------------------------------------------------------------------
bool vescValuesChanged(VescData oldVals, VescData newVals)
{
  return oldVals.ampHours != newVals.ampHours ||
         oldVals.motorCurrent != newVals.motorCurrent ||
         oldVals.missedPackets != newVals.missedPackets ||
         oldVals.unsuccessfulSends != newVals.unsuccessfulSends;
}
//------------------------------------------------------------------
bool boardTimedOut()
{
  unsigned long timeout = SEND_TO_BOARD_INTERVAL * NUM_MISSED_PACKETS_MEANS_DISCONNECTED;
  return sinceLastBoardPacketRx > (timeout + 100);
}