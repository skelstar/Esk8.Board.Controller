
void send_packet_to_board(PacketType packetType);
void manage_responses();
void manage_responses(bool success);

//------------------------------------------------------------------
void packet_available_cb(uint16_t from_id, uint8_t type)
{
  since_got_reply_from_board = 0;
  uint8_t buff[sizeof(VescData)];
  nrf24.read_into(buff, sizeof(VescData));
  memcpy(&board_packet, &buff, sizeof(VescData));

  if (board_packet.id == 0)
  {
    DEBUG("*** first packet!! ***");
    send_packet_to_board(CONFIG);
  }

  if (old_board_packet.moving != board_packet.moving)
  {
    send_to_display_event_queue(board_packet.moving ? DISP_EV_MOVING : DISP_EV_STOPPED);
  }

  old_board_packet = board_packet;

  if (display_task_initialised && commsStateTask_initialised)
  {
    send_to_comms_state_event_queue(EV_COMMS_CONNECTED);
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
    controller_packet.id++;
  }
  if (false == success)
  {
    manage_responses(false);
  }
}
//------------------------------------------------------------------

// checks board_packet.id
void manage_responses()
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
    DEBUG("disconnect because ids don't match");
  }
  manage_responses(response_ok);
}

void manage_responses(bool success)
{
  if (!comms_state_connected && success)
  {
    send_to_comms_state_event_queue(EV_COMMS_CONNECTED);
  }

  if (comms_session_started && !success)
  {
    stats.total_failed++;
#ifdef PRINT_RETRIES
    DEBUGVAL(success, stats.total_failed);
#endif
    send_to_comms_state_event_queue(EV_COMMS_DISCONNECTED);
  }
}
