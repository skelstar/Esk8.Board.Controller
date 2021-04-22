#pragma once

#include <QueueManager.h>

#ifndef __SparkFun_Qwiic_Button_H__
#include <SparkFun_Qwiic_Button.h>
#endif

#include <tasks/queues/types/PrimaryButton.h>

//------------------------------------------

namespace QwiicButtonTask
{
  RTOSTaskManager mgr("QwiicButtonTask", 3000);

  elapsedMillis since_peeked, since_checked_button, since_malloc, since_check_notf;

  QwiicButton qwiicButton;

  PrimaryButtonState state;

  Queue1::Manager<PrimaryButtonState> *primaryButtonQueue;

  const unsigned long CHECK_QUEUE_INTERVAL = 50;
  const unsigned long CHECK_BUTTON_INTERVAL = 500;

  //=====================================================

  bool takeMutex(SemaphoreHandle_t handle, TickType_t ticks = 10)
  {
    return handle != nullptr
               ? xSemaphoreTake(handle, (TickType_t)5) == pdPASS
               : false;
  }

  void giveMutex(SemaphoreHandle_t handle)
  {
    if (handle != nullptr)
      xSemaphoreGive(handle);
  }

  void task(void *pvParameters)
  {
    mgr.printStarted();

    if (mux_I2C == nullptr)
      DEBUG("WARNING! mux_I2C has not been initialised!");

    primaryButtonQueue = Queue1::Manager<PrimaryButtonState>::create("IRL primaryButtonQueue");

    bool init_button = false;
    do
    {
      if (takeMutex(mux_I2C, TICKS_500ms))
      {
        init_button = qwiicButton.begin();
        if (!init_button)
          DEBUG("Error initialising Qwiik button");
        giveMutex(mux_I2C);
      }
      vTaskDelay(200);
    } while (!init_button);
    DEBUG("Qwiic Button initialised");

    mgr.ready = true;
    mgr.printReady();

    // state.pressed = qwiicButton.isPressed();
    // primaryButtonQueue->send(&state, QueueBase::printSend);

    while (true)
    {
      if (since_check_notf > PERIOD_50ms)
      {
        since_check_notf = 0;
        if (readNotfQueue->hasValue() == Response::OK)
        {
          if (takeMutex(mux_I2C, TICKS_10ms))
          {
            state.pressed = qwiicButton.isPressed();
            giveMutex(mux_I2C);
          }
          state.event_id = readNotfQueue->payload.event_id;

          primaryButtonQueue->send(&state);
        }
      }

      mgr.healthCheck(10000);

      vTaskDelay(10);
    }

    vTaskDelete(NULL);
  }

  //============================================================
} // namespace
