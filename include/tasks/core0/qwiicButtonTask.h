#pragma once

#include <QueueManager1.h>

#ifndef __SparkFun_Qwiic_Button_H__
#include <SparkFun_Qwiic_Button.h>
#endif

#include <types/PrimaryButton.h>

//------------------------------------------

namespace QwiicButtonTask
{
  RTOSTaskManager mgr("QwiicButtonTask", 3000);

  elapsedMillis since_peeked, since_checked_button, since_malloc;

  QwiicButton qwiicButton;

  PrimaryButtonState state;

  Queue1::Manager<PrimaryButtonState> primaryButtonQueue(xSendToBoardQueueHandle, (TickType_t)5);

  const unsigned long CHECK_QUEUE_INTERVAL = 50;
  const unsigned long CHECK_BUTTON_INTERVAL = 500;

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

    mgr.ready = true;
    mgr.printReady();

    // TODO remove this? Wait for queues?
    vTaskDelay(1000);

    state.pressed = qwiicButton.isPressed();
    primaryButtonQueue.send(&state);

    while (true)
    {
      if (since_checked_button > CHECK_BUTTON_INTERVAL)
      {
        since_checked_button = 0;

        bool pressed = state.pressed;

        if (mutex_I2C.take("QwiicButtonTask: loop", TICKS_10ms))
        {
          pressed = qwiicButton.isPressed();
          mutex_I2C.give(nullptr); // 1ms
        }

        if (state.pressed != pressed)
        {
          state.pressed = pressed;
          primaryButtonQueue.send(&state);
        }
      }

      mgr.healthCheck(10000);

      vTaskDelay(10);
    }
    vTaskDelete(NULL);
  }

  //============================================================
} // namespace
