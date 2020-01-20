
void send_config_packet_to_board();

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
  if (xSPImutex != NULL && xSemaphoreTake(xSPImutex, (TickType_t)10) == pdPASS)
  {
    if (since_sent_request > 5000)
    {
      since_sent_request = 0;
      controller_packet.command = 1; // REQUEST
    }

    uint8_t bs[sizeof(ControllerData)];
    memcpy(bs, &controller_packet, sizeof(ControllerData));

    uint8_t retries = nrf24.send_with_retries(/*to*/ COMMS_BOARD, /*type*/ PacketType::CONTROL, bs, sizeof(ControllerData), NUM_RETRIES);
    retry_log.add(retries > 0);

    if (retries > 0)
    {
      DEBUGVAL(retries);
    }
    if (retries >= NUM_RETRIES)
    {
      stats.total_failed++;
    }

    controller_packet.command = 0;
    controller_packet.id++;

    xSemaphoreGive(xSPImutex);
  }
  else 
  {
    DEBUG("couldn't get semaphore! (nrf send)");
  }
}
//------------------------------------------------------------------
void send_config_packet_to_board()
{
  uint8_t bs[sizeof(ControllerConfig)];
  memcpy(bs, &controller_config, sizeof(ControllerConfig));

  if (xSPImutex != NULL && xSemaphoreTake(xSPImutex, (TickType_t)500) == pdPASS)
  {
    DEBUG("got xSPImutex! (config)");

    uint8_t retries = nrf24.send_with_retries(/*to*/ COMMS_BOARD, /*type*/ PacketType::CONFIG, bs, sizeof(ControllerConfig), NUM_RETRIES);
    if (retries > 0)
    {
      DEBUGVAL(retries);
    }
    xSemaphoreGive(xSPImutex);
  }
  else 
  {
    DEBUG("couldn't get semaphore! (config)");
  }
  controller_packet.id++;
}
