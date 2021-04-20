#pragma once

#include <types/PacketState.h>

namespace BoardCommsTask
{
#ifndef RADIO_OBJECTS
#define RADIO_OBJECTS

#include <RF24Network.h>
#include <NRF24L01Lib.h>

  NRF24L01Lib nrf24;

  RF24 radio(NRF_CE, NRF_CS);
  RF24Network network(radio);
#endif

  GenericClient<ControllerData, VescData> boardClient(COMMS_BOARD);

  // prototypes
  void boardPacketAvailable_cb(uint16_t from_id, uint8_t t);
  void boardClientInit();
  void sendConfigToBoard(bool print = false);
  void sendPacketToBoard(bool print = false);

  ControllerConfig controller_config;
  ControllerData controller_packet;

  BoardClass board;
  PacketState packetState;

  Queue1::Manager<SendToBoardNotf> *readNotfQueue = nullptr;
  Queue1::Manager<PacketState> *packetStateQueue = nullptr;

  const unsigned long CHECK_COMMS_RX_INTERVAL = 50;

  elapsedMillis
      since_checked_comms = 0,
      since_check_send_notf_queue;

  //----------------------------------------------------------
  void boardPacketAvailable_cb(uint16_t from_id, uint8_t t)
  {
    VescData packet = boardClient.read();
    board.save(packet);

    if (packet.reason == CONFIG_RESPONSE)
    {
      Serial.printf("CONFIG_RESPONSE id: %lu\n", packet.id);
    }

    packetState.received(packet);

    vTaskDelay(10);
  }
  //----------------------------------------------------------
  void sendConfigToBoard(bool print)
  {
    controller_config.send_interval = SEND_TO_BOARD_INTERVAL;
    controller_packet.id++;
    controller_packet.acknowledged = false;
    controller_config.id = controller_packet.id;

    if (print)
      Serial.printf("Sending CONFIG id: %lu\n", controller_config.id);

    bool success = boardClient.sendAltTo<ControllerConfig>(Packet::CONFIG, controller_config);

    packetState.sent(controller_config);
    packetStateQueue->send(&packetState);

    if (success == false)
      Serial.printf("Unable to send CONFIG packet to board id: %lu\n", controller_packet.id);
  }
  //----------------------------------------------------------
  void sendPacketToBoard(bool print)
  {
    controller_packet.id++;
    controller_packet.acknowledged = false;

    if (print)
      Serial.printf("sendPacketToBoard() id: %lu\n", controller_packet.id);

    bool success = boardClient.sendTo(Packet::CONTROL, controller_packet);

    packetState.sent(controller_packet);

    packetStateQueue->send(&packetState, QueueBase::printSend);

    if (success == false)
      Serial.printf("Unable to send CONTROL packet to board id: %lu\n", controller_packet.id);
  }
  //----------------------------------------------------------

  RTOSTaskManager mgr("BoardCommsTask", 10000);

  //==========================================================

  void task(void *pvParameters)
  {
    mgr.printStarted();

    controller_packet.id = 0;

    readNotfQueue = Queue1::Manager<SendToBoardNotf>::create("IRL: BoardCommsTask readNotfQueue");
    packetStateQueue = Queue1::Manager<PacketState>::create("(BoardCommsTask)packetStateQueue");

    boardClient.begin(&network, boardPacketAvailable_cb, mux_SPI);

    mgr.ready = true;
    mgr.printReady();

    if (boardClient.ready() == false)
      DEBUG("WARNING: boardClient is not ready");

    while (mgr.enabled() == false)
    {
      vTaskDelay(TICKS_5ms);
    }

    while (true)
    {
      // check readNotfQueue for when to send packet to board
      if (since_check_send_notf_queue > PERIOD_10ms)
      {
        if (readNotfQueue->hasValue())
        {
          packetState.latency = 0;
          packetState.correlationId = readNotfQueue->payload.correlationId;

          sendPacketToBoard();
        }
      }

      // check for incoming packets
      if (since_checked_comms > PERIOD_50ms)
      {
        since_checked_comms = 0;

        boardClient.update();
      }

      mgr.healthCheck(10000);

      vTaskDelay(5);
    }
    vTaskDelete(NULL);
  }
  //--------------------------------------------------------
  void boardClientInit()
  {
    // boardClient.begin(&network, boardPacketAvailable_cb, mutex_SPI.handle());
    // boardClient.setConnectedStateChangeCallback(boardConnectedState_cb);
    // boardClient.setSentPacketCallback(printSentToBoard_cb);
    // boardClient.setReadPacketCallback(printRecvFromBoard_cb);
  }
  //----------------------------------------------------------
} // namespace Remote
