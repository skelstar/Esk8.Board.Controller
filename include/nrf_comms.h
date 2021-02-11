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

  if (Board::mutex1.take(__func__))
  {
    board.save(packet);
    Board::mutex1.give(__func__);
  }

  if (Board::mutex1.take(__func__))
  {
    if (board.packet.reason == FIRST_PACKET)
    {
      DEBUG("*** board's first packet!! ***");

      Comms::queue1->send(Comms::Event::BOARD_FIRST_PACKET);

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
      displayQueue->send(DispState::UPDATE);

    else if (board.isStopped())
      displayQueue->send(DispState::STOPPED);

    else if (board.isMoving())
      displayQueue->send(DispState::MOVING);

    Board::mutex1.give(__func__);
  }

  // this should only happen when using M5STACK
  if (Board::mutex1.take(__func__))
  {
    if (DEBUG_BUILD && board.getCommand() == CommandType::RESET)
    {
      ESP.restart();
    }
    Board::mutex1.give(__func__);
  }

  Comms::queue1->send(Comms::Event::PKT_RXD);
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
  if (Board::mutex1.take(__func__, (TickType_t)TICKS_10))
  {
    bool rxLastResponse = board.packet.id == controller_packet.id - 1 &&
                          board.packet.id > 0;
    Board::mutex1.give(__func__);

    if (Stats::mutex.take(__func__, (TickType_t)TICKS_10))
    {
      if (!rxLastResponse && stats.boardConnected)
      {
        stats.total_failed_sending += 1;
        if (PRINT_IF_TOTAL_FAILED_SENDING)
          DEBUGVAL(board.packet.id, controller_packet.id);
      }
      Stats::mutex.give(__func__);
    }

    boardClient.sendTo(Packet::CONTROL, controller_packet);

    controller_packet.id++;
  }
}
//------------------------------------------------------------------

bool boardTimedOut()
{
  unsigned long timeout = SEND_TO_BOARD_INTERVAL * NUM_MISSED_PACKETS_MEANS_DISCONNECTED;
  return board.sinceLastPacket > (timeout + 100);
}
