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
  unsigned long last_qwiic_id = -1, last_throttle_id = -1;

  void init();

  void handle_qwiic_packet(QwiicButtonState *qwiic)
  {
    if (qwiic != nullptr && !qwiic->been_peeked(last_qwiic_id))
    {
      if (qwiic->missed_packet(last_qwiic_id))
        Serial.printf("[PERIPHERALS_TASK] missed at least one qwiic packet! (id: %lu, last: %lu)\n", qwiic->id, last_qwiic_id);

      myperipherals->primary_button = qwiic->pressed;
      last_qwiic_id = qwiic->id;
    }
  }

  //--------------------------------------------------------
  void task(void *pvParameters)
  {
    Serial.printf(PRINT_TASK_STARTED_FORMAT, "Peripherals", xPortGetCoreID());

    init();

    taskReady = true;

    myperipherals = new Peripherals();

    elapsedMillis since_read_peripherals;
    Peripherals *last_peripheral = new Peripherals();

    while (true)
    {
      if (since_read_peripherals > SEND_TO_BOARD_INTERVAL)
      {
        since_read_peripherals = 0;

        QwiicButtonState *qwiic = QwiicButtonTask::queue->peek<QwiicButtonState>(__func__);
        handle_qwiic_packet(qwiic);
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
