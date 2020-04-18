
void send_packet_to_board(PacketType packetType);
void checkBoardPacketId();
void manage_responses(bool success);
bool vescValuesChanged(VescData oldVals, VescData newVals);

//------------------------------------------------------------------
void packet_available_cb(uint16_t from_id, uint8_t type)
{
  sinceLastBoardPacket = 0;

  old_board_packet = board_packet;

  uint8_t buff[sizeof(VescData)];
  nrf24.read_into(buff, sizeof(VescData));
  memcpy(&board_packet, &buff, sizeof(VescData));

  if (board_packet.reason == FIRST_PACKET)
  {
    DEBUG("*** board's first packet!! ***");
    controller_packet.id = 0;

    sendToCommsEventStateQueue(EV_COMMS_BD_RESET);
    send_packet_to_board(CONFIG);
  }
  else if (old_board_packet.moving != board_packet.moving)
  {
    send_to_display_event_queue(board_packet.moving ? DISP_EV_MOVING : DISP_EV_STOPPED);
  }
  else if (vescValuesChanged(old_board_packet, board_packet))
  {
    send_to_display_event_queue(DISP_EV_UPDATE);
  }
  else
  {
    sendToCommsEventStateQueue(EV_COMMS_PKT_RXD);
  }
}
//------------------------------------------------------------------

elapsedMillis since_sent_request;

void send_packet_to_board(PacketType packetType)
{
  bool success = false;
  if (packetType == PacketType::CONTROL)
  {
    uint8_t bs[sizeof(ControllerData)];
    memcpy(bs, &controller_packet, sizeof(ControllerData));

    success = nrf24.send_packet(/*to*/ COMMS_BOARD, /*type*/ packetType, bs, sizeof(ControllerData));
    controller_packet.id++;
  }
  else if (packetType == PacketType::CONFIG)
  {
    controller_config.id = controller_packet.id;
    uint8_t bs[sizeof(ControllerConfig)];
    memcpy(bs, &controller_config, sizeof(ControllerConfig));

    success = nrf24.send_packet(/*to*/ COMMS_BOARD, /*type*/ packetType, bs, sizeof(ControllerConfig));
    // controller_packet.id++;
  }
  if (false == success)
  {
    manage_responses(false);
  }
}

//------------------------------------------------------------------

// checks board_packet.id
void checkBoardPacketId()
{
  bool response_ok = board_packet.id == controller_packet.id - 1 ||
                     board_packet.id == controller_config.id - 1;
  if (response_ok)
  {
    stats.consecutive_resps++;
    comms_session_started = stats.consecutive_resps > 3;
  }
  else
  {
    stats.consecutive_resps = 0;
    DEBUGVAL(board_packet.id, controller_packet.id, controller_config.id);
  }
  manage_responses(response_ok);
}

//------------------------------------------------------------------
void manage_responses(bool success)
{
  if (success)
  {
    if (false == comms_state_connected)
    {
      sendToCommsEventStateQueue(EV_COMMS_PKT_RXD);
    }
  }
  else
  {
    if (comms_session_started)
    {
      stats.total_failed_sending++;
#ifdef PRINT_RETRIES
      DEBUGVAL(success, stats.total_failed_sending);
#endif
      sendToCommsEventStateQueue(EV_COMMS_BOARD_TIMEDOUT);
    }
    else
    {
      DEBUGVAL(comms_session_started);
    }
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
  return sinceLastBoardPacket > ((SEND_TO_BOARD_INTERVAL * CONSECUTIVE_MISSED_PACKETS_MEANS_DISCONNECTED) + 100);
}