#pragma once

#include <SparkFun_Qwiic_Button.h>

//------------------------------------------
/* prototypes */

//------------------------------------------
class QwiicButtonState : public QueueBase
{
public:
  bool pressed = false;
} state;

namespace QwiicButtonTask
{
  /* prototypes */

  xQueueHandle queueHandle;
  Queue::Manager *queue = nullptr;

  // task
  TaskHandle_t taskHandle;
  TaskConfig config{"QwiicButtonTask", /*stack*/ 3000, taskHandle, /*ready*/ false};

  elapsedMillis since_peeked, since_checked_button, since_malloc;

  bool board_connected = false;

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

    queueHandle = xQueueCreate(/*len*/ 1, sizeof(QwiicButtonState *));
    queue = new Queue::Manager(queueHandle, (TickType_t)5);

    state.pressed = qwiicButton.isPressed();
    state.id = 0;

    config.taskReady = true;

    vTaskDelay(1000);

    queue->send(&state);
    state.id++;

    while (true)
    {
      if (since_checked_button > CHECK_BUTTON_INTERVAL)
      {
        since_checked_button = 0;

        bool pressed = state.pressed;

        if (mutex_I2C.take("qwiicButtonTask: loop", TICKS_10ms))
        {
          pressed = qwiicButton.isPressed();
          mutex_I2C.give(nullptr); // 1ms
        }

        if (state.pressed != pressed)
        {
          state.pressed = pressed;
          queue->send(&state);
          state.id++;
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

  float getStackUsage()
  {
    int highWaterMark = uxTaskGetStackHighWaterMark(config.taskHandle);
    return ((highWaterMark * 1.0) / config.stackSize) * 100.0;
  }

  int getHeapBytes()
  {
    return xPortGetFreeHeapSize();
  }

  void createTask(uint8_t core, uint8_t priority)
  {
    xTaskCreatePinnedToCore(
        task,
        "QwiicButtonTask",
        config.stackSize,
        NULL,
        priority,
        &config.taskHandle,
        core);
  }
} // namespace
