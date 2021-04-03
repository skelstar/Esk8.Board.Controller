

GenericClient<ControllerData, VescData> boardClient(COMMS_BOARD);

namespace BoardCommsTask
{
  // prototypes
  void boardPacketAvailable_cb(uint16_t from_id, uint8_t t);
  void boardClientInit();
  void sendConfigToBoard();
  void sendPacketToBoard();

  ControllerConfig controller_config;
  ControllerData controller_packet;

  BoardClass board;

  elapsedMillis sinceBoardConnected;

  //----------------------------------------------------------
  void boardPacketAvailable_cb(uint16_t from_id, uint8_t t)
  {
    VescData packet = boardClient.read();
    board.save(packet);

    if (packet.reason == FIRST_PACKET)
    {
      DEBUG("*** board's first packet!! ***");

      sendConfigToBoard();

      sinceBoardConnected = 0;
    }
    else if (packet.reason == CONFIG_RESPONSE)
    {
      Serial.printf("CONFIG_RESPONSE id: %lu\n", packet.id);
    }

    boardPacketQueue->send(&board);
  }
  //----------------------------------------------------------
  void sendConfigToBoard()
  {
    controller_config.send_interval = SEND_TO_BOARD_INTERVAL;
    controller_config.id = controller_packet.id;

    bool success = boardClient.sendAltTo<ControllerConfig>(Packet::CONFIG, controller_config);
    if (success == false)
      Serial.printf("Unable to send CONFIG packet to board id: %lu\n", controller_packet.id);
    controller_packet.id++;
  }
  //----------------------------------------------------------
  void sendPacketToBoard()
  {
    bool success = boardClient.sendTo(Packet::CONTROL, controller_packet);
    if (success == false)
      Serial.printf("Unable to send CONTROL packet to board id: %lu\n", controller_packet.id);
    controller_packet.id++;
  }
  //----------------------------------------------------------

  RTOSTaskManager mgr("BoardCommsTask", 10000);

  xQueueHandle queueHandle;
  Queue::Manager *queue = nullptr;

  const unsigned long CHECK_BOARD_CLIENT_INTERVAL = 100;
  const unsigned long CHECK_BOARD_CLIENT_DELAY = 50;

  elapsedMillis
      since_checked_comms,
      since_sent_to_board = 0 + CHECK_BOARD_CLIENT_DELAY; // 50ms ahead of sending

  //--------------------------------------------------------
  void task(void *pvParameters)
  {
    mgr.printStarted();

    controller_packet.id = 0;

    boardClientInit();

    mgr.ready = true;
    mgr.printReady();

    while (true)
    {
      if (since_sent_to_board > SEND_TO_BOARD_INTERVAL)
      {
        since_sent_to_board = 0;
        sendPacketToBoard();
      }

      if (since_checked_comms > SEND_TO_BOARD_INTERVAL)
      {
        since_checked_comms = 0;

        bool new_packet = boardClient.update();

        boardPacketQueue->send(&board);

        if (!board.connected())
        {
          Serial.printf("Board might be offline\n");
        }
      }

      // mgr.healthCheck(10000);

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
