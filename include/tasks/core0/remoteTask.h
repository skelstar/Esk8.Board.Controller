
#define BATTERY_MEASURE_INTERVAL 5000

elapsedMillis since_measure_battery;

// prototypes

namespace Remote
{
  RTOSTaskManager mgr("RemoteTask", 3000, TASK_PRIORITY_1);

  xQueueHandle queueHandle;
  Queue::Manager *queue = nullptr;

  BatteryLib battery(BATTERY_MEASURE_PIN);

  struct BatteryInfo
  {
    bool charging;
    float percent;
    float volts;
  } remote;

  //--------------------------------------------------------
  void task(void *pvParameters)
  {
    mgr.printStarted();

    Remote::battery.setup(nullptr);

    queueHandle = xQueueCreate(/*len*/ 1, sizeof(BatteryInfo));
    queue = new Queue::Manager(queueHandle, (TickType_t)5);

    mgr.ready = true;
    mgr.printReady();

    while (true)
    {
      if (since_measure_battery > BATTERY_MEASURE_INTERVAL)
      {
        since_measure_battery = 0;

        battery.update();

        remote.charging = battery.isCharging;
        remote.percent = battery.chargePercent;
        remote.volts = battery.getVolts();

        queue->send(&remote);
      }

      mgr.healthCheck(10000);

      vTaskDelay(10);
    }
    vTaskDelete(NULL);
  }
  //--------------------------------------------------------
} // namespace Remote
