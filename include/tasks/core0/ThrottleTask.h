#pragma once

#include <QueueManager.h>
#include <AS5600.h>

struct ThrottleState
{
  unsigned long id;
  uint8_t throttle;
};

namespace ThrottleTask
{
  AMS_5600 ams5600;

#include <MagThrottle.h>

  // prototypes
  void init();

  xQueueHandle queueHandle;
  Queue::Manager *queue;

  bool taskReady = false;

  elapsedMillis since_checked_throttle;

  const unsigned long CHECK_THROTTLE_INTERVAL = 100;

  unsigned long event_id = 0;

  void task(void *pvParameters)
  {
    Serial.printf(PRINT_TASK_STARTED_FORMAT, "ThrottleTask", xPortGetCoreID());

    init();

    taskReady = true;
    DEBUG("ThrottleTask ready");

    vTaskDelay(1000);

    ThrottleState state = {event_id, 127};

    while (true)
    {
      if (since_checked_throttle > CHECK_THROTTLE_INTERVAL)
      {
        since_checked_throttle = 0;

        MagneticThrottle::update();

        uint8_t throttle = MagneticThrottle::get();
        if (throttle != state.throttle)
        {
          state.id++;
          state.throttle = throttle;
          queue->send<ThrottleState>(&state);
        }
      }
      vTaskDelay(10);
    }

    vTaskDelete(NULL);
  }
  //------------------------------------------------------------

  void createTask(uint8_t core, uint8_t priority)
  {
    xTaskCreatePinnedToCore(
        task,
        "ThrottleTask",
        10000,
        NULL,
        priority,
        NULL,
        core);
  }

  void init()
  {
    queueHandle = xQueueCreate(/*len*/ 1, sizeof(ThrottleState *));
    queue = new Queue::Manager(queueHandle, (TickType_t)5);

    bool initialised = false;

    do
    {
      bool connected = MagneticThrottle::connect();

      Serial.printf("%s\n", connected
                                ? "INFO: mag-throttle connected OK"
                                : "ERROR: Could not find mag throttle");

      MagneticThrottle::init(SWEEP_ANGLE, LIMIT_DELTA_MAX, LIMIT_DELTA_MIN, THROTTLE_DIRECTION);
      MagneticThrottle::setThrottleEnabledCb([] {
        return true;
      });

      initialised = true;
      vTaskDelay(10);
    } while (!initialised);
  }
}
