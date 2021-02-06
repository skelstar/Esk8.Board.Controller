#ifndef ESK8_SHARED_TYPES_H
#include <types.h>
#endif

void processHUDPacket();
void processBoardPacket();
void sendConfigToBoard();
void sendPacketToBoard();
void sendInstructionToHud(HUD::Instruction instruction);

//------------------------------------------------------------------
void boardPacketAvailable_cb(uint16_t from_id, uint8_t t)
{
  using namespace HUD;
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
    if (FEATURE_SEND_TO_HUD)
      sendInstructionToHud(HEARTBEAT);

    controller_packet.id = 0;
    sendConfigToBoard();

    sinceBoardConnected = 0;
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

  if (board.valuesChanged())
  {
    displayQueue->send(DispState::UPDATE);
  }

  // this should only happen when using M5STACK
  if (DEBUG_BUILD && board.getCommand() == CommandType::RESET)
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

  if (!FEATURE_SEND_TO_HUD)
    return;

  uint16_t ev = hudClient.read();
  if ((uint16_t)ev > HUDAction::Length)
  {
    Serial.printf("WARNING: Action from HUD out of range (%d)\n", (uint16_t)ev);
    return;
  }

  // TODO respond with appropriate action response?

  switch (HUDAction::Event(ev))
  {
  case HUDAction::ONE_CLICK:
    hudActionQueue->send(ev);
    break;
  default:
    if (FEATURE_SEND_TO_HUD)
      sendInstructionToHud(TWO_FLASHES | GREEN | FAST);
  }
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

void sendInstructionToHud(HUD::Instruction instruction)
{
  if (!FEATURE_SEND_TO_HUD)
    return;
  // TODO: this online check probably shouldn't be in here
  if (hudClient.connected())
  {
    hudClient.sendTo(Packet::HUD, instruction);
  }
  else
  {
    if (DEBUG_BUILD)
      Serial.printf("WARNING: instruction not sent because hud offline\n");
  }
}

//------------------------------------------------------------------

bool boardTimedOut()
{
  unsigned long timeout = SEND_TO_BOARD_INTERVAL * NUM_MISSED_PACKETS_MEANS_DISCONNECTED;
  return board.sinceLastPacket > (timeout + 100);
}
