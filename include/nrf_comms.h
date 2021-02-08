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

  if (board.packet.reason == FIRST_PACKET)
  {
    /*
    * send board reset event to commsState
    * set controller_packet.id = 0
    * send controller "CONFIG" packet to board
    */
    DEBUG("*** board's first packet!! ***");

    nrfCommsQueue->send(Comms::Event::BOARD_FIRST_PACKET);

    controller_packet.id = 0;
    sendConfigToBoard();

    sinceBoardConnected = 0;

    Stats::queue->send(Stats::BOARD_FIRST_PACKET);
  }
  else if (board.startedMoving())
  {
    displayQueue->send(DispState::MOVING);
    Stats::queue->send(Stats::MOVING);
  }
  else if (board.hasStopped())
  {
    displayQueue->send(DispState::STOPPED);
    Stats::queue->send(Stats::STOPPED);
  }
  else if (board.valuesChanged())
  {
    displayQueue->send(DispState::UPDATE);
  }
  else
  {
    uint8_t command = board.isStopped()
                          ? DispState::STOPPED
                          : DispState::MOVING;
    displayQueue->send(command);
  }

  // this should only happen when using M5STACK
  if (DEBUG_BUILD && board.getCommand() == CommandType::RESET)
  {
    ESP.restart();
  }

  nrfCommsQueue->send(Comms::Event::PKT_RXD);
}
//------------------------------------------------------------------

void sendConfigToBoard()
{
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

bool boardTimedOut()
{
  unsigned long timeout = SEND_TO_BOARD_INTERVAL * NUM_MISSED_PACKETS_MEANS_DISCONNECTED;
  return board.sinceLastPacket > (timeout + 100);
}
