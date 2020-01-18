void send_config_packet_to_board();

void packet_available_cb(uint16_t from_id, uint8_t type)
{
  uint8_t buff[sizeof(VescData)];
  nrf24.read_into(buff, sizeof(VescData));
  memcpy(&board_packet, &buff, sizeof(VescData));

  if (board_packet.id == 0)
  {
    DEBUG("*** first packet!! ***");
    
    send_config_packet_to_board();
    // COMMS_EVENT(EV_COMMS_FIRST_PACKET, "EV_COMMS_FIRST_PACKET");
  }

  switch (board_packet.reason)
  {
    case ReasonType::BOARD_STOPPED:
      DEBUG("***Stopped!***");
      break;

    case ReasonType::BOARD_MOVING:
      DEBUG("***Moving!***");
      break;
      
    default:
      DEBUGVAL(from_id, board_packet.id, since_sent_to_board);
      break;
  }
}
//------------------------------------------------------------------

elapsedMillis since_sent_request;

void send_control_packet_to_board()
{
  if (since_sent_request > 5000)
  {
    since_sent_request = 0;
    controller_packet.command = 1; // REQUEST
  }

  DEBUGVAL("sending...", controller_packet.id);

  uint8_t bs[sizeof(ControllerData)];
  memcpy(bs, &controller_packet, sizeof(ControllerData));

  uint8_t retries = nrf24.send_with_retries(/*to*/ COMMS_BOARD, /*type*/PacketType::CONTROL, bs, sizeof(ControllerData), NUM_RETRIES);
  if (retries > 0)
  {
    DEBUGVAL(retries);
  }
  controller_packet.command = 0;
  controller_packet.id++;
}
//------------------------------------------------------------------
void send_config_packet_to_board()
{
  uint8_t bs[sizeof(ControllerConfig)];
  memcpy(bs, &controller_config, sizeof(ControllerConfig));

  uint8_t retries = nrf24.send_with_retries(/*to*/ COMMS_BOARD, /*type*/PacketType::CONFIG, bs, sizeof(ControllerConfig), NUM_RETRIES);
  if (retries > 0)
  {
    DEBUGVAL(retries);
  }
  controller_packet.id++;
}

