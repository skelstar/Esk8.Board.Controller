#pragma once

namespace nsPeripherals
{
  Peripherals *myperipherals;
  namespace
  {
    const char *taskName = "Peripherals";
  }

  uint8_t getThrottle(uint8_t raw_throttle, uint8_t primary);

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
    unsigned long last_qwiic_id = -1, last_throttle_id = -1;
    Peripherals *last_peripheral = new Peripherals();

    while (true)
    {
      if (since_read_peripherals > SEND_TO_BOARD_INTERVAL)
      {
        since_read_peripherals = 0;

        QwiicButtonState *res = QwiicButtonTask::queue->peek<QwiicButtonState>(__func__);
        if (res != nullptr && res->id != last_qwiic_id)
        {
          DEBUG("qwiic button res != NULL");
          if (res->id > last_qwiic_id + 1)
            Serial.printf("[PERIPHERALS_TASK] missed at least one packet! (id: %lu, last: %lu)\n", res->id, last_qwiic_id);

          if (res->id != last_qwiic_id)
          {
            myperipherals->primary_button = res->pressed;
            last_qwiic_id = res->id;

            DEBUGVAL(myperipherals->primary_button);
          }
        }

        ThrottleState *throttle = ThrottleTask::queue->peek<ThrottleState>("PeripheralsTask loop");
        if (throttle != nullptr && last_throttle_id != throttle->id)
        {
          DEBUG("throttle res != NULL");
          myperipherals->throttle = getThrottle(throttle->val, myperipherals->primary_button);
          last_throttle_id = throttle->id;
        }

        bool changed = last_peripheral->throttle != myperipherals->throttle ||
                       last_peripheral->primary_button != myperipherals->primary_button;
        if (changed)
        {
          myperipherals->throttle = throttle->val;
          myperipherals->event = Event::EV_THROTTLE;
          myperipherals->id++;
          peripheralsQueue->send(myperipherals);
          DEBUGVAL("sent:", myperipherals->throttle, myperipherals->primary_button);
        }
        last_peripheral = new Peripherals(*myperipherals);
      }

      vTaskDelay(10);
    }
    vTaskDelete(NULL);
  }
  //--------------------------------------------------------

  uint8_t getThrottle(uint8_t raw_throttle, uint8_t deadman)
  {
    if (raw_throttle <= 127)
      return raw_throttle;
    else if (deadman == 1)
      return raw_throttle;
    return 127;
  }

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
      Serial.printf("button %s pressed\n", NintendoClassicTask::getButtonName(button));

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
      Serial.printf("button %s released\n", NintendoClassicTask::getButtonName(button));

    uint8_t *buttons = classic.get_buttons();
    for (int i = 0; i < NintendoController::BUTTON_COUNT; i++)
      myperipherals->classicButtons[i] = buttons[i];

    myperipherals->event = EV_CLASSIC_BUTTON;
    myperipherals->id++;
    peripheralsQueue->send(&myperipherals);
  }

  void init()
  {
    // NintendoClassicTask::init();

    // MagneticThrottle::init();

    // classic.setButtonPressedCb(nintendoButtonPressed_cb);
    // classic.setButtonReleasedCb(nintendoButtonReleased_cb);

    // if (OPTION_USING_MAG_THROTTLE)
    // {
    // bool connected = MagneticThrottle::connect();

    // Serial.printf("%s\n", connected
    //                           ? "INFO: mag-throttle connected OK"
    //                           : "ERROR: Could not find mag throttle");

    // MagneticThrottle::setThrottleEnabledCb([] {
    //   return myperipherals->primary_button == 1; // qwiicButton.isPressed();
    // });
    // MagneticThrottle::init(SWEEP_ANGLE, LIMIT_DELTA_MAX, LIMIT_DELTA_MIN, THROTTLE_DIRECTION);
    // }
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
