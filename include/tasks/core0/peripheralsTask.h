#pragma once

#include <NintendoController.h>
#include <NintendoButtons.h>

namespace nsPeripherals
{
  Peripherals *myperipherals;
  namespace
  {
    const char *taskName = "Peripherals";
  }

  bool taskReady = false;

  void init();

  //--------------------------------------------------------
  void task(void *pvParameters)
  {
    Serial.printf(PRINT_TASK_STARTED_FORMAT, "Peripherals", xPortGetCoreID());

    init();

    taskReady = true;

    myperipherals = new Peripherals();

    elapsedMillis since_read_peripherals;
    unsigned long last_id = -1;

    while (true)
    {
      if (since_read_peripherals > SEND_TO_BOARD_INTERVAL)
      {
        since_read_peripherals = 0;

        // check bus first
        QwiickButtonState *res = QwiicButtonTask::queue->peek<QwiickButtonState>(__func__);
        if (res != nullptr)
        {
          if (res->id > last_id + 1)
            Serial.printf("[PERIPHERALS_TASK] missed at least one packet! (id: %lu, last: %lu)\n", res->id, last_id);

          if (res->id != last_id)
          {
            myperipherals->primary_button = res->pressed;
            last_id = res->id;

            DEBUGVAL(myperipherals->primary_button);
          }
        }

        peripheralsQueue->clear();

        bool changed = false;

        // throttle
        MagThrottle::update();
        // classic buttons
        ClassicButtons::loop();

        changed = changed ||
                  myperipherals->throttle != MagThrottle::get();

        uint8_t throttle = MagThrottle::get();

        myperipherals->throttle = throttle;
        myperipherals->event = Event::EV_THROTTLE;
        myperipherals->id++;
        peripheralsQueue->send(myperipherals);
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
      myperipherals->classicButtons[i] = buttons[i];

    myperipherals->event = EV_CLASSIC_BUTTON;
    myperipherals->id++;
    peripheralsQueue->send(&myperipherals);
  }

  void nintendoButtonReleased_cb(uint8_t button)
  {
    if (PRINT_NINTENDO_BUTTON)
      Serial.printf("button %s released\n", ClassicButtons::getButtonName(button));

    uint8_t *buttons = classic.get_buttons();
    for (int i = 0; i < NintendoController::BUTTON_COUNT; i++)
      myperipherals->classicButtons[i] = buttons[i];

    myperipherals->event = EV_CLASSIC_BUTTON;
    myperipherals->id++;
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
        return myperipherals->primary_button == 1; // qwiicButton.isPressed();
      });
      MagThrottle::init(SWEEP_ANGLE, LIMIT_DELTA_MAX, LIMIT_DELTA_MIN, THROTTLE_DIRECTION);
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
