
//=-----------------------------------------
/* prototypes */

//------------------------------------------
struct QwiickButtonState
{
  bool pressed;
  unsigned long id;
} state;

namespace QwiicButtonTask
{
  /* prototypes */

  xQueueHandle queueHandle;
  Queue::Manager *queue;

  bool taskReady = false;

  elapsedMillis since_peeked, since_checked_button;

  bool board_connected = false;
  unsigned long last_id = 0;

  QwiicButton qwiicButton;

  const unsigned long CHECK_QUEUE_INTERVAL = 50;
  const unsigned long CHECK_BUTTON_INTERVAL = 100;

  //=====================================================

  void task(void *pvParameters)
  {
    Serial.printf(PRINT_TASK_STARTED_FORMAT, "Qwiic Button Task", xPortGetCoreID());

    bool init_button = false;
    do
    {
      if (mutex_I2C.take("QwiicButtonTask: init", (TickType_t)500))
      {
        init_button = qwiicButton.begin();
        if (!init_button)
          DEBUG("Error initialising Qwiik button");
        mutex_I2C.give(__func__);
      }
      vTaskDelay(200);
    } while (!init_button);
    DEBUG("Qwiic Button initialised");

    queueHandle = xQueueCreate(/*len*/ 1, sizeof(QwiickButtonState *));
    queue = new Queue::Manager(queueHandle, (TickType_t)5);

    state.pressed = qwiicButton.isPressed();
    state.id = 0;

    taskReady = true;

    vTaskDelay(1000);

    queue->send(&state);
    state.id++;

    while (true)
    {
      if (since_checked_button > CHECK_BUTTON_INTERVAL)
      {
        since_checked_button = 0;

        bool pressed = state.pressed;

        if (mutex_I2C.take(nullptr, TICKS_10))
        {
          pressed = qwiicButton.isPressed();
          mutex_I2C.give(nullptr);
        }

        if (state.pressed != pressed)
        {
          state.pressed = pressed;
          queue->send(&state);
          state.id++;
          DEBUGVAL("Qwiic button event", pressed, state.pressed);
        }
      }

      vTaskDelay(10);
    }
    vTaskDelete(NULL);
  }

  //============================================================

  void init()
  {
    // DEBUG("qwiic init()");
    // vTaskDelay(100);
    // bool success = qwiicButton.begin();
    // if (mutex_I2C.take(__func__, portMAX_DELAY))
    // {
    //   DEBUG("took mutex (init)");
    // if (false == qwiicButton.begin())
    //   DEBUG("Error initialising Qwiik button");
    // mutex_I2C.give(__func__);
    // }
  }

  //------------------------------------------------------------

  void createTask(uint8_t core, uint8_t priority)
  {
    xTaskCreatePinnedToCore(
        task,
        "QwiicButtonTask",
        10000,
        NULL,
        priority,
        NULL,
        core);
  }
} // namespace Debug
