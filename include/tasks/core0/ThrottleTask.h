#pragma once

#include <QueueManager1.h>
#include <types/PrimaryButton.h>

//----------------------------------------
class ThrottleState : public QueueBase
{
public:
  uint8_t val = 127;
};
//----------------------------------------

namespace ThrottleTask
{
  RTOSTaskManager mgr("ThrottleTask", 3000);

  // prototypes
  void init();

  elapsedMillis since_checked_throttle;

  const unsigned long CHECK_THROTTLE_INTERVAL = 200;

  PrimaryButtonState primary_button;

  //================================================
  void task(void *pvParameters)
  {
    mgr.printStarted();

    Queue1::Manager<PrimaryButtonState> primaryBtnQueue(xPrimaryButtonQueue, (TickType_t)5);

    init();

    vTaskDelay(100);

    ThrottleState throttle;

    mgr.ready = true;
    mgr.printReady();

    while (true)
    {
      if (since_checked_throttle > CHECK_THROTTLE_INTERVAL)
      {
        since_checked_throttle = 0;

        if (primaryBtnQueue.hasValue(__func__))
        {
          primary_button = primaryBtnQueue.value;
          Serial.printf("buttonQueue: pressed=%d\n", primary_button.pressed);
        }

        MagneticThrottle::update();

        uint8_t raw_throttle = MagneticThrottle::get();
        if (raw_throttle != throttle.val)
        {
          throttle.val = raw_throttle;
          throttleQueue->send<ThrottleState>(&throttle);
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

      MagneticThrottle::init(SWEEP_ANGLE, LIMIT_DELTA_MAX, LIMIT_DELTA_MIN, THROTTLE_DIRECTION);
      MagneticThrottle::setThrottleEnabledCb([] {
        return primary_button.pressed;
      });

      initialised = true;
      vTaskDelay(10);
    } while (!initialised);
  }
}
