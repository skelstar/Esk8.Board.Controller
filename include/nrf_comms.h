
void send_config_packet_to_board();
void manage_retries(uint8_t retries);

//------------------------------------------------------------------
void packet_available_cb(uint16_t from_id, uint8_t type)
{
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
    send_config_packet_to_board();
  }

#ifdef FEATURE_PUSH_TO_ENABLE
  throttle_enabled = board_packet.moving;
#endif

  switch (board_packet.reason)
  {
  case ReasonType::BOARD_STOPPED:
    DEBUG("***Stopped!***");
    send_to_display_event_queue(DISP_EV_STOPPED);
    break;

  case ReasonType::BOARD_MOVING:
    DEBUG("***Moving!***");
    send_to_display_event_queue(DISP_EV_MOVING);
    break;

  default:
    DEBUGVAL(from_id, board_packet.id, since_sent_to_board);
    break;
  }

  comms_state_event(EV_COMMS_CONNECTED);
}
//------------------------------------------------------------------

elapsedMillis since_sent_request;

void send_control_packet_to_board()
{
  if (since_sent_request > 5000 || comms_state_connected == false)
  {
    since_sent_request = 0;
    controller_packet.command = 1; // REQUEST
  }

  uint8_t bs[sizeof(ControllerData)];
  memcpy(bs, &controller_packet, sizeof(ControllerData));

  uint8_t retries = nrf24.send_with_retries(/*to*/ COMMS_BOARD, /*type*/ PacketType::CONTROL, bs, sizeof(ControllerData), NUM_RETRIES);

  if (retries)
  {
    DEBUGVAL(retries);
  }
  manage_retries(retries);

  controller_packet.command = 0;
  controller_packet.id++;
}
//------------------------------------------------------------------
void send_config_packet_to_board()
{
  uint8_t bs[sizeof(ControllerConfig)];
  memcpy(bs, &controller_config, sizeof(ControllerConfig));

  uint8_t retries = nrf24.send_with_retries(/*to*/ COMMS_BOARD, /*type*/ PacketType::CONFIG, bs, sizeof(ControllerConfig), NUM_RETRIES);
  if (retries > 0)
  {
    DEBUGVAL(retries);
  }
  controller_packet.id++;
}
//------------------------------------------------------------------

float old_retry_rate = 0.0;

void manage_retries(uint8_t retries)
{
  if (comms_session_started)
  {
    retry_log.add(retries > 0);

    float retry_rate = retry_log.get();

    if (retries > 0)
    {
      stats.num_packets_with_retries++;
      send_to_display_event_queue(DISP_EV_REFRESH);
      if (retries >= NUM_RETRIES)
      {
        stats.total_failed++;
        comms_state_event(EV_COMMS_DISCONNECTED);
      }
    }
    else if (old_retry_rate != retry_rate)
    {
      old_retry_rate = retry_rate;
      send_to_display_event_queue(DISP_EV_REFRESH);
    }
  }
}
