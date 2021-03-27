#pragma once

#include <QueueManager.h>
#include <AS5600.h>

//----------------------------------------
class ThrottleState : public QueueBase
{
public:
  uint8_t val = 127;
};
//----------------------------------------

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

  const unsigned long CHECK_THROTTLE_INTERVAL = 66;

  unsigned long event_id = 0;
  QwiicButtonState primary_button;

  void task(void *pvParameters)
  {
    Serial.printf(PRINT_TASK_STARTED_FORMAT, "ThrottleTask", xPortGetCoreID());

    init();

    DEBUG("ThrottleTask waiting for QwiicButtonTask to be ready");
    while (!QwiicButtonTask::taskReady)
      vTaskDelay(100);

    taskReady = true;
    DEBUG("ThrottleTask ready");

    vTaskDelay(1000);

    ThrottleState throttle;

    while (true)
    {
      if (since_checked_throttle > CHECK_THROTTLE_INTERVAL)
      {
        since_checked_throttle = 0;

        QwiicButtonState *button = QwiicButtonTask::queue->peek<QwiicButtonState>(__func__);
        if (button != nullptr && primary_button.id != button->id)
        {
          primary_button.id = button->id;
          primary_button.pressed = button->pressed;
        }

        MagneticThrottle::update();

        uint8_t raw_throttle = MagneticThrottle::get();
        if (raw_throttle != throttle.val)
        {
          throttle.id++;
          throttle.val = raw_throttle;
          queue->send<ThrottleState>(&throttle);
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
