#pragma once

#include <Wire.h>
#include <NintendoController.h>
#include <QueueManager.h>

NintendoController classic;

class NintendoButtonEvent : public QueueBase
{
public:
  uint8_t button;
  uint8_t state;
};

namespace NintendoClassicTask
{
  // prototypes
  uint8_t button_changed(uint8_t *new_buttons, uint8_t *old_buttons);
  void init();

  xQueueHandle queueHandle;
  Queue::Manager *queue;

  // task
  TaskHandle_t taskHandle;
  TaskConfig config = {"NintendoClassicTask", 3000, taskHandle, /*taskReady*/ true};

  bool taskReady = false;

  elapsedMillis since_checked_buttons, since_health_check, since_task_created;

  const unsigned long CHECK_BUTTONS_INTERVAL = 100;

  unsigned long event_id = 0;

  uint8_t last_buttons[NintendoController::BUTTON_COUNT];

  //=====================================================
  void task(void *pvParameters)
  {
    NintendoButtonEvent ev;

    Serial.printf(PRINT_TASK_STARTED_FORMAT, config.name, xPortGetCoreID());

    init();

    DEBUG("Classic Buttons initialised");

    taskReady = true;
    Serial.printf("Ready after %lums of being created\n", (unsigned long)since_task_created);

    while (true)
    {
      if (since_checked_buttons > CHECK_BUTTONS_INTERVAL)
      {
        since_checked_buttons = 0;

        bool updated = classic.update(mutex_I2C.handle(), TICKS_50ms);
        if (updated)
        {
          uint8_t *new_buttons = classic.get_buttons();
          uint8_t button_that_changed = button_changed(new_buttons, last_buttons);
          if (button_that_changed != 99)
          {
            // something changed, send
            event_id++;

            ev.id = event_id;
            ev.button = button_that_changed;
            ev.state = new_buttons[button_that_changed];
            // Serial.printf("Something changed: button %d state: %d id: %lu\n", ev.button, ev.state, ev.id);

            queue->send(&ev);
          }
          // save
          for (int i = 0; i < NintendoController::BUTTON_COUNT; i++)
          {
            last_buttons[i] = new_buttons[i];
          }
        }
      }

      // if (since_health_check > 3000)
      // {
      //   since_health_check = 0;
      //   Serial.printf("HEALTHCHECK: NintendoClassicTask\n");
      // }

      vTaskDelay(10);
    }
    vTaskDelete(NULL);
  }
  //------------------------------------------------------------

  float getStackUsage()
  {
    int highWaterMark = uxTaskGetStackHighWaterMark(taskHandle);
    return ((highWaterMark * 1.0) / config.stackSize) * 100.0;
  }

  void createTask(uint8_t core, uint8_t priority)
  {
    since_task_created = 0;
    xTaskCreatePinnedToCore(
        task,
        config.name,
        config.stackSize,
        NULL,
        priority,
        &config.taskHandle,
        core);
  }

  void deleteTask(bool print = false)
  {
    if (print)
    {
      Serial.printf("-----------------------------\n");
      Serial.printf("DELETING %s!\n", config.name);
      Serial.printf("-----------------------------\n");
    }
    vTaskDelete(config.taskHandle);
  }

  const char *getButtonName(uint8_t button)
  {
    switch (button)
    {
    case NintendoController::BUTTON_UP:
      return "BUTTON_UP";
    case NintendoController::BUTTON_RIGHT:
      return "BUTTON_RIGHT";
    case NintendoController::BUTTON_DOWN:
      return "BUTTON_DOWN";
    case NintendoController::BUTTON_LEFT:
      return "BUTTON_LEFT";
    case NintendoController::BUTTON_A:
      return "BUTTON_A";
    case NintendoController::BUTTON_B:
      return "BUTTON_B";
    case NintendoController::BUTTON_START:
      return "BUTTON_START";
    case NintendoController::BUTTON_SELECT:
      return "BUTTON_SELECT";
    case NintendoController::BUTTON_COUNT:
      return "BUTTON_COUNT";
    }
    return "OUT OF RANGE: getButtonName()";
  }

  NintendoController::ButtonEventCallback _pressed_cb = nullptr, _released_cb = nullptr;

  uint8_t button_changed(uint8_t *new_buttons, uint8_t *old_buttons)
  {
    for (int i = 0; i < NintendoController::BUTTON_COUNT; i++)
      if (old_buttons[i] != new_buttons[i])
        return i;
    return 99;
  }

  void init()
  {
    queueHandle = xQueueCreate(/*len*/ 1, sizeof(NintendoController *));
    queue = new Queue::Manager(queueHandle, (TickType_t)5);

    bool initialised = false;

    do
    {
      if (mutex_I2C.take("NintendoButtons: init()", TICKS_50ms))
      {
        if (!classic.init())
          Serial.printf("ERROR: could not find Nintendo Buttons\n");
        mutex_I2C.give(__func__);
        initialised = true;
      }
      vTaskDelay(10);
    } while (!initialised);
  }
}