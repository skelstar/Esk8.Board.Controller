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
  RTOSTaskManager mgr("QwiicButtonTask", 3000);

  xQueueHandle queueHandle;
  Queue::Manager *queue = nullptr;

  elapsedMillis since_peeked, since_checked_button, since_malloc;

  QwiicButton qwiicButton;

  const unsigned long CHECK_QUEUE_INTERVAL = 50;
  const unsigned long CHECK_BUTTON_INTERVAL = 100;

  //=====================================================

  void task(void *pvParameters)
  {
    mgr.printStarted();

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
    state.event_id = 0;

    mgr.ready = true;
    mgr.printReady();

    // TODO remove this? Wait for queues?
    vTaskDelay(1000);

    queue->sendLegacy(&state);
    state.event_id++;

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
          queue->sendLegacy(&state);
          state.event_id++;
        }
      }

      mgr.healthCheck(10000);

      vTaskDelay(10);
    }
    vTaskDelete(NULL);
  }

  //============================================================

  void init()
  {
  }
} // namespace
