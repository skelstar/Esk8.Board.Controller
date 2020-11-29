#ifndef ESK8_SHARED_TYPES_H
#include <types.h>
#endif

void processHUDPacket();
void processBoardPacket();
void sendConfigToBoard();
void sendPacketToBoard();
bool sendPacket(uint8_t *d, uint8_t len, uint8_t packetType);
bool sendCommandToHud(HUDCommand::Mode mode, HUDCommand::Colour colour, bool print = false);

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
  HUDAction::Event ev = readFromNrf<HUDAction::Event>();
  if (ev == HUDAction::HEARTBEAT)
  {
    hud.connected = sendCommandToHud(HUDCommand::MODE_NONE, HUDCommand::BLACK);
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

    controller_packet.id = 0;
    sendConfigToBoard();

    sinceBoardConnected = 0;
  }
  else if (board.startedMoving())
  {
    displayQueue->send(DispState::MOVING);
    sendCommandToHud(HUDCommand::FLASH, HUDCommand::BLUE);
  }
  else if (board.hasStopped())
  {
    displayQueue->send(DispState::STOPPED);
    sendCommandToHud(HUDCommand::MODE_NONE, HUDCommand::BLACK);
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
    DEBUGVAL(board.packet.id, controller_packet.id);
  }

  vTaskSuspendAll();
  sendTo<ControllerData>(COMMS_BOARD, Packet::CONTROL, controller_packet);
  xTaskResumeAll();

  controller_packet.id++;
}
//------------------------------------------------------------------

bool sendCommandToHud(HUDCommand::Mode mode, HUDCommand::Colour colour, bool print)
{
  HUDData packet(mode, colour);
  packet.id = hudData.id++;
  elapsedMillis sinceSendingToHud = 0;

  bool success = sendTo<HUDData>(COMMS_HUD, Packet::HUD, packet);
  if (print)
    Serial.printf(
        "--> HUD: mode=%s colour=%s\n",
        HUDCommand::modeNames[(int)packet.mode],
        HUDCommand::colourName[(int)packet.colour]);
  return success;
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
