
void send_packet_to_board(PacketType packetType);
void manage_retries(bool success);

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
#ifdef FEATURE_CRUISE_CONTROL
    controller_config.cruise_control_enabled = true;
#else
    controller_config.cruise_control_enabled = false;
#endif
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
    uint8_t bs[sizeof(ControllerConfig)];
    memcpy(bs, &controller_config, sizeof(ControllerConfig));

    success = nrf24.send_packet(/*to*/ COMMS_BOARD, /*type*/ packetType, bs, sizeof(ControllerConfig));
    controller_packet.id++;
  }
  if (false == success)
  {
    manage_retries(false);
  }
}
//------------------------------------------------------------------

void manage_retries(bool success)
{
  if (comms_session_started && !success)
  {
    stats.total_failed++;
#ifdef PRINT_RETRIES
    DEBUGVAL(success, stats.total_failed);
#endif
    // send_to_comms_state_event_queue(EV_COMMS_DISCONNECTED);
    send_to_display_event_queue(DISP_EV_REFRESH);
  }
}
