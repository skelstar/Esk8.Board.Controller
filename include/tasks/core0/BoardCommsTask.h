

#define PERIOD_10MS 10
#define PERIOD_20MS 20
#define PERIOD_30MS 30
#define PERIOD_40MS 40
#define PERIOD_50MS 50
#define PERIOD_100MS 100
#define PERIOD_200MS 200

namespace BoardCommsTask
{
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

  const unsigned long CHECK_COMMS_RX_INTERVAL = 50;

  elapsedMillis
      // since_sent_to_board = SEND_TO_BOARD_INTERVAL - 500,
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
    boardPacketQueue->send(&board);

    packetState.received(packet);
    packetStateQueue->send(&packetState);
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
    packetStateQueue->send(&packetState, "sendConfigToBoard");

    if (success == false)
      Serial.printf("Unable to send CONFIG packet to board id: %lu\n", controller_packet.id);
  }
  //----------------------------------------------------------
  void sendPacketToBoard(bool print)
  {
    controller_packet.id++;
    controller_packet.acknowledged = false;

    if (print)
      Serial.printf("Sending CONTROL id: %lu\n", controller_packet.id);

    bool success = boardClient.sendTo(Packet::CONTROL, controller_packet);

    packetState.sent(controller_packet);

    packetStateQueue->send(&packetState);

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

    Queue1::Manager<SendToBoardNotf> sendToBoardQueue(xSendToBoardQueueHandle, TICKS_5ms);

    boardClientInit();

    mgr.ready = true;
    mgr.printReady();

    while (mgr.enabled() == false)
    {
      vTaskDelay(500);
    }

    unsigned long notf_id = -1;

    while (true)
    {
      // check sendToBoardQueue for when to send packet to board
      if (since_check_send_notf_queue > PERIOD_10MS)
      {
        if (sendToBoardQueue.hasValue(__func__))
        {
          sendPacketToBoard();
        }
      }

      // check for incoming packets
      if (since_checked_comms > PERIOD_100MS)
      {
        since_checked_comms = 0;

        bool new_packet = boardClient.update();

        packetStateQueue->send(&packetState);
      }

      mgr.healthCheck(10000);

      vTaskDelay(10);
    }
    vTaskDelete(NULL);
  }
  //--------------------------------------------------------
  void boardClientInit()
  {
    boardClient.begin(&network, boardPacketAvailable_cb, mutex_SPI.handle());
    // boardClient.setConnectedStateChangeCallback(boardConnectedState_cb);
    // boardClient.setSentPacketCallback(printSentToBoard_cb);
    // boardClient.setReadPacketCallback(printRecvFromBoard_cb);
  }
  //----------------------------------------------------------
} // namespace Remote
