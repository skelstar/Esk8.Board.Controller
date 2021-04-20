#pragma once

#include <Wire.h>
#include <types/NintendoButtonEvent.h>
#ifndef NintendoController
#include <NintendoController.h>
#endif

namespace NintendoClassicTask
{
  NintendoController classic;

  RTOSTaskManager mgr("NintendoClassicTask", 3000);

  // prototypes
  uint8_t button_changed(uint8_t *new_buttons, uint8_t *old_buttons);
  void init();

  elapsedMillis since_checked_buttons, since_health_check, since_task_created, since_checked_notf;

  const unsigned long CHECK_BUTTONS_INTERVAL = 100;

  uint8_t last_buttons[NintendoController::BUTTON_COUNT];

  //=====================================================
  void task(void *pvParameters)
  {
    mgr.printStarted();

    NintendoButtonEvent ev;

    init();

    DEBUG("Classic Buttons initialised");

    mgr.ready = true;
    mgr.printReady();

    while (true)
    {
      if (since_checked_notf > PERIOD_50ms)
      {
        since_checked_notf = 0;
        // if (waitForNew(readSendNotfQueue, PERIOD_10ms) == Response::OK)
        // {

        //   // bool changed = classic.update(mutex_I2C.handle(), TICKS_50ms);
        //   // if (changed)
        //   // {
        //   //   uint8_t *new_buttons = classic.get_buttons();
        //   //   uint8_t button_that_changed = button_changed(new_buttons, last_buttons);
        //   //   if (button_that_changed != 99)
        //   //   {
        //   //     // something changed, send
        //   //     ev.button = button_that_changed;
        //   //     ev.state = new_buttons[button_that_changed];
        //   //   }
        //   //   // save
        //   //   for (int i = 0; i < NintendoController::BUTTON_COUNT; i++)
        //   //   {
        //   //     last_buttons[i] = new_buttons[i];
        //   //   }
        //   // }
        //   // ev.event_id = readSendNotfQueue->payload.event_id;
        //   // nintendoControllerQueue->send(&ev);
        // }
      }

      // if (since_checked_buttons > CHECK_BUTTONS_INTERVAL)
      // {
      //   since_checked_buttons = 0;

      //   bool changed = classic.update(mutex_I2C.handle(), TICKS_50ms);
      //   if (changed)
      //   {
      //     uint8_t *new_buttons = classic.get_buttons();
      //     uint8_t button_that_changed = button_changed(new_buttons, last_buttons);
      //     if (button_that_changed != 99)
      //     {
      //       // something changed, send
      //       ev.button = button_that_changed;
      //       ev.state = new_buttons[button_that_changed];

      //       nintendoControllerQueue->send(&ev);
      //     }
      //     // save
      //     for (int i = 0; i < NintendoController::BUTTON_COUNT; i++)
      //     {
      //       last_buttons[i] = new_buttons[i];
      //     }
      //   }
      // }

      mgr.healthCheck(10000);

      vTaskDelay(10);
    }
    vTaskDelete(NULL);
  }
  //------------------------------------------------------------

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

  // NintendoController::ButtonEventCallback _pressed_cb = nullptr, _released_cb = nullptr;

  uint8_t button_changed(uint8_t *new_buttons, uint8_t *old_buttons)
  {
    for (int i = 0; i < NintendoController::BUTTON_COUNT; i++)
      if (old_buttons[i] != new_buttons[i])
        return i;
    return NintendoController::BUTTON_NONE;
  }

  void init()
  {
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