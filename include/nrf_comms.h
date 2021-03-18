#ifndef ESK8_SHARED_TYPES_H
#include <types.h>
#endif

void processBoardPacket();
void sendConfigToBoard();
void sendPacketToBoard();

const char *getReason(ReasonType reason)
{
  switch (reason)
  {
  case BOARD_STOPPED:
    return "BOARD_STOPPED";
  case BOARD_MOVING:
    return "BOARD_MOVING";
  case FIRST_PACKET:
    return "FIRST_PACKET";
  case LAST_WILL:
    return "LAST_WILL";
  case REQUESTED:
    return "REQUESTED";
  case VESC_OFFLINE:
    return "VESC_OFFLINE";
  case RESPONSE:
    return "RESPONSE";
  }
  return "Out of range (getReason())";
}

//------------------------------------------------------------------
void boardPacketAvailable_cb(uint16_t from_id, uint8_t t)
{
  sinceLastBoardPacketRx = 0;

  VescData packet = boardClient.read();

  board.save(packet);

  // send to other tasks
  if (boardPacketQueue != NULL)
    boardPacketQueue->send(&board);

  if (board.packet.reason == FIRST_PACKET)
  {
    DEBUG("*** board's first packet!! ***");

    controller_packet.id = 0;
    sendConfigToBoard();

    sinceBoardConnected = 0;
  }

  // this should only happen when using M5STACK
  if (DEBUG_BUILD && board.getCommand() == CommandType::RESET)
  {
    ESP.restart();
  }
}
//------------------------------------------------------------------

void sendConfigToBoard()
{
  if (PRINT_TX_TO_BOARD)
    Serial.printf("Sending config to board\n");
  controller_config.send_interval = SEND_TO_BOARD_INTERVAL;
  controller_config.id = controller_packet.id;
  boardClient.sendAltTo<ControllerConfig>(Packet::CONFIG, controller_config);
}
//------------------------------------------------------------------

void sendPacketToBoard()
{
  bool rxLastResponse = board.packet.id == controller_packet.id - 1 &&
                        board.packet.id > 0;

  if (!rxLastResponse && stats.boardConnected)
  {
    stats.total_failed_sending += 1;
    if (PRINT_IF_TOTAL_FAILED_SENDING)
      DEBUGVAL(board.packet.id, controller_packet.id);
  }

  boardClient.sendTo(Packet::CONTROL, controller_packet);

  controller_packet.id++;
}
//------------------------------------------------------------------

void updateThrottle(nsPeripherals::Peripherals *periph)
{
  if (periph == nullptr)
  {
    Serial.printf("ERROR: periph is null\n");
    return;
  }

  if (periph->throttle > 127 && periph->primary_button == 1)
    controller_packet.throttle = periph->throttle;
  else if (periph->throttle > 127)
    controller_packet.throttle = 127;
  else
    controller_packet.throttle = periph->throttle;

  if (PRINT_THROTTLE)
    DEBUGVAL(controller_packet.throttle);
}
