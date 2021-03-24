
//=-----------------------------------------
/* prototypes */

//------------------------------------------

namespace SparkFunButton
{
  /* prototypes */

  bool taskReady = false;

  elapsedMillis since_peeked, since_checked_button;

  bool board_connected = false;
  unsigned long last_id = 0;

  nsPeripherals::Peripherals *peripherals;

  QwiicButton qwiicButton;

  const unsigned long CHECK_QUEUE_INTERVAL = 50;
  const unsigned long CHECK_BUTTON_INTERVAL = 100;

  //=====================================================

  void task(void *pvParameters)
  {
    Serial.printf(PRINT_TASK_STARTED_FORMAT, "Spark Fun Button Task", xPortGetCoreID());

    while (nsPeripherals::taskReady == false)
    {
      vTaskDelay(10);
    }

    if (mutex_I2C.take(__func__, portMAX_DELAY))
    {
      if (false == qwiicButton.begin())
        DEBUG("Error initialising Qwiik button");
      mutex_I2C.give(__func__);
    }

    peripherals = new nsPeripherals::Peripherals();

    // init();

    taskReady = true;
    unsigned long last_id = -1;

    while (true)
    {
      if (since_checked_button > CHECK_BUTTON_INTERVAL)
      {
        since_checked_button = 0;

        ulong now = millis();

        bool pressed = peripherals->primary_button;
        if (mutex_I2C.take(nullptr, TICKS_10))
        {
          pressed = qwiicButton.isPressed();
          mutex_I2C.give(nullptr);
        }

        if (peripherals->primary_button != pressed)
        {
          peripherals->primary_button = pressed;
          peripherals->id++;
          peripherals->event = nsPeripherals::Event::EV_PRIMARY_BUTTON;
          peripheralsQueue->send(peripherals);
          last_id = peripherals->id;
          DEBUGVAL("Qwiic button event", pressed, peripherals->primary_button);
        }
      }

      if (since_peeked > CHECK_QUEUE_INTERVAL)
      {
        since_peeked = 0;

        nsPeripherals::Peripherals *res = peripheralsQueue->peek<nsPeripherals::Peripherals>(__func__);

        if (res != nullptr)
        {
          if (res->id > last_id + 1)
            Serial.printf("[SPARKFUN_BUTTON_TASK] missed at least one packet! (id: %lu, last: %lu)\n", res->id, last_id);

          if (res->id != last_id)
          {
            peripherals = new nsPeripherals::Peripherals(*res);
            last_id = peripherals->id;
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
        "SparkFunButtonTask",
        10000,
        NULL,
        priority,
        NULL,
        core);
  }
} // namespace Debug
