#pragma once

#include <QueueManager1.h>
#include <types/PrimaryButton.h>
#include <utils.h>
#include <types/Throttle.h>

#ifndef MAGNETIC_THROTTLE_H
#include <MagThrottle.h>>
#endif

//----------------------------------------

namespace ThrottleTask
{
  RTOSTaskManager mgr("ThrottleTask", 3000);

  // prototypes
  void init();

  elapsedMillis since_checked_throttle;

  const unsigned long CHECK_THROTTLE_INTERVAL = 200;

  PrimaryButtonState primary_button;

  bool throttleEnabled_cb()
  {
    return primary_button.pressed;
  }

  //================================================
  void task(void *pvParameters)
  {
    mgr.printStarted();

    Queue1::Manager<PrimaryButtonState> primaryBtnQueue(xPrimaryButtonQueue, (TickType_t)5);
    primaryBtnQueue.setMissedEventCallback([](uint16_t num_events) {
      Serial.printf("WARNING: missed %d events in primaryBtnQueue (ThrottleTask)\n", num_events);
    });

    Queue1::Manager<SendToBoardNotf> sendToBoardNotf(xSendToBoardQueueHandle, (TickType_t)5);

    Queue1::Manager<ThrottleState> throttleQueue1(xThrottleQueue, (TickType_t)5);

    MagneticThrottle::init(SWEEP_ANGLE, LIMIT_DELTA_MAX, LIMIT_DELTA_MIN, THROTTLE_DIRECTION);
    MagneticThrottle::setThrottleEnabledCb(throttleEnabled_cb);

    init();

    vTaskDelay(100);

    ThrottleState throttle;

    mgr.ready = true;
    mgr.printReady();

    while (true)
    {

      if (sendToBoardNotf.hasValue())
      {
        since_checked_throttle = 0;

        if (primaryBtnQueue.hasValue("ThrottleTask"))
        {
          primary_button = primaryBtnQueue.value;
        }

        MagneticThrottle::update();

        uint8_t raw_throttle = MagneticThrottle::get();
        throttle.val = raw_throttle;
        throttleQueue1.send(&throttle, [](ThrottleState t) {
          // Serial.printf("sending throttle to queue t: %d\n", t.val);
        });
      }

      mgr.healthCheck(5000);

      vTaskDelay(10);
    }

    vTaskDelete(NULL);
  }

  void init()
  {
    primary_button.pressed = false;

    bool initialised = false;

    do
    {
      bool connected = MagneticThrottle::connect();

      Serial.printf("%s\n", connected
                                ? "INFO: mag-throttle connected OK"
                                : "ERROR: Could not find mag throttle");

      initialised = true;
      vTaskDelay(10);
    } while (!initialised);
  }
}
