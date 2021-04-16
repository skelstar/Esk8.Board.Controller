#pragma once

#include <QueueManager1.h>
#include <types/PrimaryButton.h>
#include <utils.h>
#include <types/Throttle.h>

#ifndef PRINT_THROTTLE
#define PRINT_THROTTLE 0
#endif

#ifndef MAGNETIC_THROTTLE_H
#include <MagThrottle.h>
#endif

//----------------------------------------

namespace ThrottleTask
{
  RTOSTaskManager mgr("ThrottleTask", 5000);

  // prototypes
  void init();

  elapsedMillis since_checked_throttle, since_checked_for_notf;

  const unsigned long CHECK_THROTTLE_INTERVAL = 200;

  Queue1::Manager<SendToBoardNotf> *readNotfQueue = nullptr;
  Queue1::Manager<ThrottleState> *throttleStateQueue = nullptr;
  Queue1::Manager<PrimaryButtonState> *primaryButton = nullptr;

  PrimaryButtonState primary_button;

  bool throttleEnabled_cb()
  {
    return primary_button.pressed;
  }

  Queue1::Manager<ThrottleState> *createQueueManager(const char *name)
  {
    return new Queue1::Manager<ThrottleState>(xThrottleQueueHandle, TICKS_5ms, name);
  }

  //================================================
  void task(void *pvParameters)
  {
    mgr.printStarted();

    readNotfQueue = SendToBoardTimerTask::createQueueManager("readNotf");
    throttleStateQueue = createQueueManager("IRL ThrottleState");
    primaryButton = QwiicButtonTask::createQueueManager("IRL PrimaryButtonState");

    // primaryButton->setMissedEventCallback([](uint16_t num_events) {
    //   Serial.printf("WARNING: missed %d events in primaryBtnQueue (ThrottleTask)\n", num_events);
    // });

    // MagneticThrottle::init(SWEEP_ANGLE, LIMIT_DELTA_MAX, LIMIT_DELTA_MIN, THROTTLE_DIRECTION);
    // MagneticThrottle::setThrottleEnabledCb(throttleEnabled_cb);

    // init();

    vTaskDelay(100);

    ThrottleState throttle;

    mgr.ready = true;
    mgr.printReady();

    while (true)
    {
      if (since_checked_for_notf > 50)
      {
        if (readNotfQueue->hasValue())
        {
          DEBUG("Throttle has notf");
          // since_checked_throttle = 0;
          //     throttleStateQueue->payload.correlationId = readNotfQueue->payload.correlationId;

          //     if (primaryButton->hasValue())
          //     {
          //       primary_button.pressed = primaryButton->payload.pressed;
          //     }

          //     MagneticThrottle::update();

          //     uint8_t raw_throttle = MagneticThrottle::get();
          //     throttle.val = raw_throttle;
          //     throttleStateQueue->send_r(&throttle, QueueBase::printSend);
        }
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
