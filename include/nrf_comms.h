#ifndef ESK8_SHARED_TYPES_H
#include <types.h>
#endif

void processHUDPacket();
void processBoardPacket();
void sendConfigToBoard();
void sendPacketToBoard();
bool sendPacket(uint8_t *d, uint8_t len, uint8_t packetType);
bool sendCommandToHud(HUDCommand::Mode mode, HUDCommand::Colour colour, HUDCommand::Speed speed, uint8_t number = 1, bool print = true);
bool sendMessageToHud(HUDTask::Message message, bool print = true);

template <typename T>
T readFromNrf();
template <typename T>
bool sendTo(uint8_t who, Packet::Type t, T data);

//------------------------------------------------------------------
void packetAvailable_cb(uint16_t from_id, uint8_t t)
{
  if (t == (uint8_t)Packet::HUD)
  {
    sinceLastHudPacket = 0;
    hud.connected = true;
    processHUDPacket();
  }
  else
  {
    sinceLastBoardPacketRx = 0;
    processBoardPacket();
    nrfCommsQueue->send(CommsState::PKT_RXD);
  }
}
//------------------------------------------------------------------

void processHUDPacket()
{
  hud.connected = true;
  HUDAction::Event ev = readFromNrf<HUDAction::Event>();

  switch (ev)
  {
  case HUDAction::HEARTBEAT:
    hud.connected = sendMessageToHud(HUDTask::HEARTBEAT);
    break;
  case HUDAction::ONE_CLICK:
    hud.connected = sendMessageToHud(HUDTask::ACKNOWLEDGE);
    break;
  case HUDAction::TWO_CLICK:
    hud.connected = sendMessageToHud(HUDTask::CYCLE_BRIGHTNESS);
    break;
  case HUDAction::THREE_CLICK:
    hud.connected = sendMessageToHud(HUDTask::THREE_FLASHES);
    break;
  default:
    hud.connected = sendMessageToHud(HUDTask::ACKNOWLEDGE);
    break;
  }
  if (PRINT_HUD_COMMS)
    Serial.printf("<-- HUD: %s\n", HUDAction::names[(int)ev]);
}
//------------------------------------------------------------------
void processBoardPacket()
{
  VescData packet = readFromNrf<VescData>();

  board.save(packet);

  if (board.packet.reason == FIRST_PACKET)
  {
    /*
    * send board reset event to commsState
    * set controller_packet.id = 0
    * send controller "CONFIG" packet to board
    */
    DEBUG("*** board's first packet!! ***");

    nrfCommsQueue->send(CommsState::BOARD_FIRST_PACKET);
    hud.connected = sendMessageToHud(HUDTask::Message::BOARD_CONNECTED);

    controller_packet.id = 0;
    sendConfigToBoard();

    sinceBoardConnected = 0;
  }
  else if (board.startedMoving())
  {
    displayQueue->send(DispState::MOVING);
    hud.connected = sendMessageToHud(HUDTask::BOARD_MOVING);
  }
  else if (board.hasStopped())
  {
    displayQueue->send(DispState::STOPPED);
    hud.connected = sendMessageToHud(HUDTask::BOARD_STOPPED);
  }
  else if (board.valuesChanged())
  {
    displayQueue->send(DispState::UPDATE);
  }

  if (board.getCommand() == CommandType::RESET)
  {
    ESP.restart();
  }
}

//------------------------------------------------------------------

void sendConfigToBoard()
{
  controller_config.id = controller_packet.id;
  uint8_t bs[sizeof(ControllerConfig)];
  memcpy(bs, &controller_config, sizeof(ControllerConfig));
  uint8_t len = sizeof(ControllerConfig);

  sendPacket(bs, len, Packet::CONFIG);
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

  vTaskSuspendAll();
  sendTo<ControllerData>(COMMS_BOARD, Packet::CONTROL, controller_packet);
  xTaskResumeAll();

  controller_packet.id++;
}
//------------------------------------------------------------------

bool sendCommandToHud(HUDCommand::Mode mode, HUDCommand::Colour colour, HUDCommand::Speed speed, uint8_t number, bool print)
{
  if (hud.connected)
  {
    HUDData packet(mode, colour, speed, number);
    packet.id = hudData.id++;
    return sendTo<HUDData>(COMMS_HUD, Packet::HUD, packet);
  }
  else
  {
    Serial.printf("WARNING: command not sent because hud offline\n");
  }
  return false;
}
//------------------------------------------------------------------

bool sendMessageToHud(HUDTask::Message message, bool print)
{
  if (hud.connected)
  {
    if (print && PRINT_HUD_COMMS)
      Serial.printf("-->HUD: %s\n", HUDTask::messageName[(int)message]);

    switch (message)
    {
    case HUDTask::BOARD_DISCONNECTED:
      return sendCommandToHud(HUDCommand::Mode::PULSE, HUDCommand::Colour::GREEN, HUDCommand::FAST, print);
    case HUDTask::BOARD_CONNECTED:
      return sendCommandToHud(HUDCommand::Mode::MODE_NONE, HUDCommand::Colour::BLACK, HUDCommand::NO_SPEED, print);
    case HUDTask::WARNING_ACK:
      return sendCommandToHud(HUDCommand::Mode::PULSE, HUDCommand::Colour::RED, HUDCommand::SLOW, print);
    case HUDTask::CONTROLLER_RESET:
      return sendCommandToHud(HUDCommand::Mode::SPIN, HUDCommand::Colour::RED, HUDCommand::MED, print);
    case HUDTask::BOARD_MOVING:
      return sendCommandToHud(HUDCommand::FLASH, HUDCommand::GREEN, HUDCommand::FAST, 1, print);
    case HUDTask::BOARD_STOPPED:
      return sendCommandToHud(HUDCommand::FLASH, HUDCommand::RED, HUDCommand::MED, 1, print);
    case HUDTask::HEARTBEAT:
      return sendCommandToHud(HUDCommand::FLASH, HUDCommand::BLUE, HUDCommand::MED, 1, print);
    case HUDTask::ACKNOWLEDGE:
      return sendCommandToHud(HUDCommand::FLASH, HUDCommand::GREEN, HUDCommand::FAST, 2, print);
    case HUDTask::CYCLE_BRIGHTNESS:
      return sendCommandToHud(HUDCommand::CYCLE_BRIGHTNESS, HUDCommand::BLACK, HUDCommand::NO_SPEED, print);
    case HUDTask::THREE_FLASHES:
      return sendCommandToHud(HUDCommand::FLASH, HUDCommand::RED, HUDCommand::FAST, 3, print);
    case HUDTask::GO_TO_IDLE:
      return sendCommandToHud(HUDCommand::MODE_NONE, HUDCommand::BLACK, HUDCommand::NO_SPEED, print);
    default:
      return true;
    }
  }
  else
  {
    Serial.printf("WARNING: message not sent because hud offline\n");
  }
  return hud.connected;
}

//------------------------------------------------------------------

bool sendPacket(uint8_t *d, uint8_t len, uint8_t packetType)
{
  bool sent = nrf24.send(COMMS_BOARD, packetType, d, len);

  return sent;
}
//------------------------------------------------------------------

bool boardTimedOut()
{
  unsigned long timeout = SEND_TO_BOARD_INTERVAL * NUM_MISSED_PACKETS_MEANS_DISCONNECTED;
  return board.sinceLastPacket > (timeout + 100);
}

//------------------------------------------------------------------

template <typename T>
T readFromNrf()
{
  T ev;
  uint8_t buff[sizeof(T)];
  nrf24.read_into(buff, sizeof(T));
  memcpy(&ev, &buff, sizeof(T));
  return ev;
}
//------------------------------------------------------------------
template <typename T>
bool sendTo(uint8_t who, Packet::Type t, T data)
{
  uint8_t len = sizeof(T);
  uint8_t bs[len];
  memcpy(bs, &data, len);
  // takes 3ms if OK, 30ms if not OK
  return nrf24.send(who, t, bs, len);
}
//------------------------------------------------------------------
