#ifndef ESK8_SHARED_TYPES_H
#include <types.h>
#endif

void processHUDPacket();
void processBoardPacket();
void sendConfigToBoard();
void sendPacketToBoard();
bool sendPacket(uint8_t *d, uint8_t len, uint8_t packetType);
void sendCommandToHud(uint16_t command);

//------------------------------------------------------------------
void boardPacketAvailable_cb(uint16_t from_id, uint8_t t)
{
  using namespace HUD;
  sinceLastBoardPacketRx = 0;

  VescData packet = boardClient.read(PRINT_RX_FROM_BOARD);

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
    sendCommandToHud(1 << HEARTBEAT);

    controller_packet.id = 0;
    sendConfigToBoard();

    sinceBoardConnected = 0;
  }
  else if (board.startedMoving())
  {
    displayQueue->send(DispState::MOVING);
    sendCommandToHud(1 << FAST | 1 << GREEN | 1 << FLASH);
  }
  else if (board.hasStopped())
  {
    displayQueue->send(DispState::STOPPED);
    sendCommandToHud(1 << SLOW | 1 << RED | 1 << FLASH);
  }
  else if (board.valuesChanged())
  {
    displayQueue->send(DispState::UPDATE);
  }

  if (board.getCommand() == CommandType::RESET)
  {
    ESP.restart();
  }

  nrfCommsQueue->send(Comms::Event::PKT_RXD);
}
//------------------------------------------------------------------

void hudPacketAvailable_cb(uint16_t from_id, uint8_t type)
{
  using namespace HUD;
  if (type != Packet::HUD)
  {
    Serial.printf("WARNING: Rx type: %d not supported!\n", type);
    return;
  }

  HUDAction::Event ev = hudClient.read(PRINT_RX_FROM_HUD);
  if ((uint16_t)ev > HUDAction::Length)
  {
    Serial.printf("WARNING: Action from HUD out of range (%d)\n", (uint16_t)ev);
    return;
  }

  // TODO respond with appropriate action response?
  sendCommandToHud(1 << TWO_FLASHES | 1 << BLUE | 1 << FAST);
}

//------------------------------------------------------------------

HUDTask::Message mapToHUDTask(HUDAction::Event ev)
{
  switch (ev)
  {
  case HUDAction::HEARTBEAT:
    return HUDTask::HEARTBEAT;
  case HUDAction::ONE_CLICK:
    return HUDTask::ACKNOWLEDGE;
  case HUDAction::TWO_CLICK:
    return HUDTask::CYCLE_BRIGHTNESS;
  case HUDAction::THREE_CLICK:
    return HUDTask::THREE_FLASHES;
  default:
    return HUDTask::ACKNOWLEDGE;
  }
}

//------------------------------------------------------------------

void sendConfigToBoard()
{
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

  boardClient.sendTo(Packet::CONTROL, controller_packet, PRINT_TX_TO_BOARD);

  controller_packet.id++;
}
//------------------------------------------------------------------

void sendCommandToHud(uint16_t command)
{
  if (hudClient.connected())
  {
    hudClient.sendTo(Packet::HUD, command, PRINT_TX_TO_HUD);
  }
  else
  {
    Serial.printf("WARNING: command not sent because hud offline\n");
  }
}

//------------------------------------------------------------------

bool sendPacket(uint8_t *d, uint8_t len, uint8_t packetType)
{
  // bool sent = nrf24.send(COMMS_BOARD, packetType, d, len);

  return true; // sent;
}
//------------------------------------------------------------------

bool boardTimedOut()
{
  unsigned long timeout = SEND_TO_BOARD_INTERVAL * NUM_MISSED_PACKETS_MEANS_DISCONNECTED;
  return board.sinceLastPacket > (timeout + 100);
}
