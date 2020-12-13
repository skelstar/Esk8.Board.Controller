#ifndef ESK8_SHARED_TYPES_H
#include <types.h>
#endif

void processHUDPacket();
void processBoardPacket();
void sendConfigToBoard();
void sendPacketToBoard();
bool sendPacket(uint8_t *d, uint8_t len, uint8_t packetType);
void sendCommandToHud(HUDCommand::Mode mode, HUDCommand::Colour colour, HUDCommand::Speed speed, uint8_t number = 1, bool print = true);
void sendMessageToHud(HUDTask::Message message, bool print = true);
HUDTask::Message mapToHUDTask(HUDAction::Event action);

//------------------------------------------------------------------
void boardPacketAvailable_cb(uint16_t from_id, uint8_t t)
{
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
    sendMessageToHud(HUDTask::Message::BOARD_CONNECTED);

    controller_packet.id = 0;
    sendConfigToBoard();

    sinceBoardConnected = 0;
  }
  else if (board.startedMoving())
  {
    displayQueue->send(DispState::MOVING);
    sendMessageToHud(HUDTask::BOARD_MOVING);
  }
  else if (board.hasStopped())
  {
    displayQueue->send(DispState::STOPPED);
    sendMessageToHud(HUDTask::BOARD_STOPPED);
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
  if (type != Packet::HUD)
  {
    Serial.printf("WARNING: Rx type: %d not supported!\n", type);
    return;
  }

  HUDAction::Event ev = hudClient.read(PRINT_RX_FROM_HUD);
  if ((uint8_t)ev > HUDAction::Length)
  {
    Serial.printf("WARNING: Action from HUD out of range (%d)\n", (uint8_t)ev);
    return;
  }

  HUDTask::Message message = mapToHUDTask(ev);
  sendMessageToHud(message);
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

void sendCommandToHud(HUDCommand::Mode mode, HUDCommand::Colour colour, HUDCommand::Speed speed, uint8_t number, bool print)
{
  if (hudClient.connected())
  {
    HUDData packet(mode, colour, speed, number);
    packet.id = hudData.id++;
    hudClient.sendTo(Packet::HUD, packet, PRINT_TX_TO_HUD);
  }
  else
  {
    Serial.printf("WARNING: command not sent because hud offline\n");
  }
}
//------------------------------------------------------------------

void sendMessageToHud(HUDTask::Message message, bool print)
{
  if (hudClient.connected())
  {
    switch (message)
    {
    case HUDTask::BOARD_DISCONNECTED:
      sendCommandToHud(HUDCommand::Mode::PULSE, HUDCommand::Colour::GREEN, HUDCommand::FAST, print);
      break;
    case HUDTask::BOARD_CONNECTED:
      sendCommandToHud(HUDCommand::Mode::MODE_NONE, HUDCommand::Colour::BLACK, HUDCommand::NO_SPEED, print);
      break;
    case HUDTask::WARNING_ACK:
      sendCommandToHud(HUDCommand::Mode::PULSE, HUDCommand::Colour::RED, HUDCommand::SLOW, print);
      break;
    case HUDTask::CONTROLLER_RESET:
      sendCommandToHud(HUDCommand::Mode::SPIN, HUDCommand::Colour::RED, HUDCommand::MED, print);
      break;
    case HUDTask::BOARD_MOVING:
      sendCommandToHud(HUDCommand::FLASH, HUDCommand::GREEN, HUDCommand::FAST, 1, print);
      break;
    case HUDTask::BOARD_STOPPED:
      sendCommandToHud(HUDCommand::FLASH, HUDCommand::RED, HUDCommand::MED, 1, print);
      break;
    case HUDTask::HEARTBEAT:
      sendCommandToHud(HUDCommand::FLASH, HUDCommand::BLUE, HUDCommand::MED, 1, print);
      break;
    case HUDTask::ACKNOWLEDGE:
      sendCommandToHud(HUDCommand::FLASH, HUDCommand::GREEN, HUDCommand::FAST, 2, print);
      break;
    case HUDTask::CYCLE_BRIGHTNESS:
      sendCommandToHud(HUDCommand::CYCLE_BRIGHTNESS, HUDCommand::BLACK, HUDCommand::NO_SPEED, print);
      break;
    case HUDTask::THREE_FLASHES:
      sendCommandToHud(HUDCommand::FLASH, HUDCommand::RED, HUDCommand::FAST, 3, print);
      break;
    case HUDTask::GO_TO_IDLE:
      sendCommandToHud(HUDCommand::MODE_NONE, HUDCommand::BLACK, HUDCommand::NO_SPEED, print);
      break;
    default:
      return;
    }
  }
  else
  {
    Serial.printf("WARNING: message not sent because hud offline\n");
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
