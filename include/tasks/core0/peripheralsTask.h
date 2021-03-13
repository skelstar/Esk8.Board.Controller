
namespace nsPeripherals
{
  Peripherals myperipherals;
  namespace
  {
    const char *taskName = "Peripherals";
  }

  QwiicButton qwiicButton;

  bool taskReady = false;

  void init();

  //--------------------------------------------------------
  void task(void *pvParameters)
  {
    Serial.printf(PRINT_TASK_STARTED_FORMAT, "Peripherals", xPortGetCoreID());

    init();

    taskReady = true;

    elapsedMillis since_read_peripherals;

    while (true)
    {
      if (since_read_peripherals > 200)
      {
        since_read_peripherals = 0;

        bool changed = false;
        bool pressed = qwiicButton.isPressed();
        changed = myperipherals.primary_button != pressed;
        myperipherals.primary_button = qwiicButton.isPressed();

        if (changed)
        {
          DEBUG("sending myperipherals");
          mgPeripherals->send(&myperipherals);
        }
      }

      vTaskDelay(10);
    }
    vTaskDelete(NULL);
  }
  //--------------------------------------------------------

  void init()
  {
    if (OPTION_USING_MAG_THROTTLE)
    {
      // if (!MagThrottle::connected())
      //   Serial.printf("ERROR: Could not find mag throttle\n");
      qwiicButton.begin();

      // MagThrottle::init(SWEEP_ANGLE, LIMIT_DELTA, THROTTLE_DIRECTION);
      // MagThrottle::setThrottleEnabledCb([] {
      //   return qwiicButton.isPressed();
      // });
    }
  }

  //--------------------------------------------------------
  void createTask(uint8_t core, uint8_t priority)
  {
    taskName = "Battery Measure";
    xTaskCreatePinnedToCore(
        task,
        taskName,
        10000,
        NULL,
        priority,
        NULL,
        core);
  }
} // namespace Peripherals
