
//=-----------------------------------------
/* prototypes */

//------------------------------------------

namespace Debug
{
  /* prototypes */

  bool taskReady = false;

  elapsedMillis since_peeked;

  bool board_connected = false;
  unsigned long last_id = 0;

  //=====================================================

  void task(void *pvParameters)
  {
    Serial.printf(PRINT_TASK_STARTED_FORMAT, "Debug Task", xPortGetCoreID());

    init();

    taskReady = true;

    while (true)
    {
      if (since_peeked > 50)
      {
        since_peeked = 0;

        BoardClass *board = boardPacketQueue->peek<BoardClass>();
        if (board != nullptr)
        {
          if (board->id != last_id)
          {
            if (!board_connected && board->connected())
              DEBUG("DEBUG: board connected!");
            else if (board_connected && !board->connected())
              DEBUG("DEBUG: board disconnected!");
            board_connected = board->connected();

            if (board->id - last_id > 1)
              Serial.printf("[DEBUG-TASK] missed at least one packet! (id: %lu, last: %lu)\n", board->id, last_id);
            last_id = board->id;
          }
        }
      }

      vTaskDelay(10);
    }
    vTaskDelete(NULL);
  }

  //============================================================

  void init()
  {
  }

  //------------------------------------------------------------

  void createTask(uint8_t core, uint8_t priority)
  {
    xTaskCreatePinnedToCore(
        task,
        "debugTask",
        10000,
        NULL,
        priority,
        NULL,
        core);
  }
} // namespace Debug
