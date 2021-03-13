#pragma once

#include <NintendoController.h>
#include <NintendoButtons.h>

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
        // primary button
        bool primary_pressed = qwiicButton.isPressed();
        // throttle
        MagThrottle::update();
        // classic buttons
        ClassicButtons::loop();

        // uint8_t *btn = classic.get_buttons();
        // for (int i = 0; i < NintendoController::BUTTON_COUNT; i++)
        // {
        //   changed == changed || myperipherals.classicButtons[i] != btn[i];
        //   myperipherals.classicButtons[i] = btn[i];
        // }

        // if (changed)
        //   DEBUG("changed");

        changed = changed ||
                  myperipherals.primary_button != primary_pressed ||
                  myperipherals.throttle != MagThrottle::get();
        myperipherals.primary_button = primary_pressed;
        myperipherals.throttle = MagThrottle::get();

        mgPeripherals->clear();
        if (changed)
        {
          // DEBUGVAL(myperipherals.primary_button, myperipherals.throttle);
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
    ClassicButtons::init();

    if (OPTION_USING_MAG_THROTTLE)
    {
      if (!MagThrottle::connect())
        Serial.printf("ERROR: Could not find mag throttle\n");
      qwiicButton.begin();

      MagThrottle::setThrottleEnabledCb([] {
        return myperipherals.primary_button == 1; // qwiicButton.isPressed();
      });
      MagThrottle::init(SWEEP_ANGLE, LIMIT_DELTA, THROTTLE_DIRECTION);
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
