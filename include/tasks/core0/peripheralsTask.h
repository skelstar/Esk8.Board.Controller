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
      if (since_read_peripherals > SEND_TO_BOARD_INTERVAL)
      {
        since_read_peripherals = 0;

        peripheralsQueue->clear();

        bool changed = false;
        // primary button
        bool primary_pressed = qwiicButton.isPressed();
        // throttle
        MagThrottle::update();
        // classic buttons
        ClassicButtons::loop();

        changed = changed ||
                  myperipherals.primary_button != primary_pressed ||
                  myperipherals.throttle != MagThrottle::get();

        uint8_t throttle = MagThrottle::get();

        if (myperipherals.primary_button != primary_pressed)
        {
          myperipherals.primary_button = primary_pressed;
          myperipherals.event = Event::EV_PRIMARY_BUTTON;
          peripheralsQueue->send(&myperipherals);
        }
        else if (myperipherals.throttle != throttle)
        {
          myperipherals.throttle = throttle;
          myperipherals.event = Event::EV_THROTTLE;
          peripheralsQueue->send(&myperipherals);
        }
      }

      vTaskDelay(10);
    }
    vTaskDelete(NULL);
  }
  //--------------------------------------------------------

  void print_buttons(uint8_t *buttons)
  {
    Serial.printf("buttons: ");
    for (int i = 0; i < NintendoController::BUTTON_COUNT; i++)
      Serial.printf("%d", buttons[i]);
    Serial.printf("\n");
  }

  void nintendoButtonPressed_cb(uint8_t button)
  {
    if (PRINT_NINTENDO_BUTTON)
      Serial.printf("button %s pressed\n", ClassicButtons::getButtonName(button));

    uint8_t *buttons = classic.get_buttons();
    for (int i = 0; i < NintendoController::BUTTON_COUNT; i++)
      myperipherals.classicButtons[i] = buttons[i];

    myperipherals.event = EV_CLASSIC_BUTTON;
    peripheralsQueue->send(&myperipherals);
  }

  void nintendoButtonReleased_cb(uint8_t button)
  {
    if (PRINT_NINTENDO_BUTTON)
      Serial.printf("button %s released\n", ClassicButtons::getButtonName(button));

    uint8_t *buttons = classic.get_buttons();
    for (int i = 0; i < NintendoController::BUTTON_COUNT; i++)
      myperipherals.classicButtons[i] = buttons[i];

    myperipherals.event = EV_CLASSIC_BUTTON;
    peripheralsQueue->send(&myperipherals);
  }

  void init()
  {
    ClassicButtons::init();

    classic.setButtonPressedCb(nintendoButtonPressed_cb);
    classic.setButtonReleasedCb(nintendoButtonReleased_cb);

    if (OPTION_USING_MAG_THROTTLE)
    {
      bool connected = MagThrottle::connect();

      Serial.printf("%s\n", connected
                                ? "INFO: mag-throttle connected OK"
                                : "ERROR: Could not find mag throttle");

      MagThrottle::setThrottleEnabledCb([] {
        return myperipherals.primary_button == 1; // qwiicButton.isPressed();
      });
      MagThrottle::init(SWEEP_ANGLE, LIMIT_DELTA_MAX, LIMIT_DELTA_MIN, THROTTLE_DIRECTION);
    }
    qwiicButton.begin();
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
