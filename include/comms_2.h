
void packet_available_cb(uint16_t from_id, uint8_t type)
{
  uint8_t buff[sizeof(VescData)];
  nrf24.read_into(buff, sizeof(VescData));
  memcpy(&board_packet, &buff, sizeof(VescData));

  if (board_packet.id == 0)
  {
    DEBUG("*** first packet!! ***");
    // COMMS_EVENT(EV_COMMS_FIRST_PACKET, "EV_COMMS_FIRST_PACKET");
  }

  DEBUGVAL(from_id, board_packet.id, since_sent_to_board);

  // COMMS_EVENT(EV_COMMS_PACKET, "EV_COMMS_PACKET");
}
//------------------------------------------------------------------

elapsedMillis since_sent_request;

void send_to_board()
{
  if (since_sent_request > 5000)
  {
    since_sent_request = 0;
    controller_packet.command = 1; // REQUEST
  }

  DEBUGVAL("sending...", controller_packet.id);

  uint8_t bs[sizeof(ControllerData)];
  memcpy(bs, &controller_packet, sizeof(ControllerData));

  uint8_t retries = nrf24.send_with_retries(/*to*/ COMMS_BOARD, /*type*/ 0, bs, sizeof(ControllerData), NUM_RETRIES);
  if (retries > 0)
  {
    DEBUGVAL(retries);
  }
  controller_packet.command = 0;
  controller_packet.id++;
}