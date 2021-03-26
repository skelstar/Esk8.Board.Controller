#include <Arduino.h>
#include <unity.h>

#define DEBUG_SERIAL 1

#ifdef DEBUG_SERIAL
#define DEBUG_OUT Serial
#endif
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#define PRINT_STATS_MUTEX_TAKE_STATE 1

#include <rtosManager.h>
#include <QueueManager.h>
#include <SparkFun_Qwiic_Button.h>
#include <elapsedMillis.h>

MyMutex mutex_I2C;

xQueueHandle queueHandle;
Queue::Manager *queue;

#define PRINT_TASK_STARTED_FORMAT "TASK: %s on Core %d\n"

#define TICKS_5 5
#define TICKS_10 10

#include <tasks/core0/qwiicButtonTask.h>

#define CORE_0 0
#define PRIORITY_1 1

elapsedMillis since_checked_queue;

// runs every test
void setUp()
{
}

// runs every test
void tearDown()
{
}

void test_qwiic_queue_sent()
{
  QwiickButtonState *actual;

  bool found = false;
  while (found == false)
  {
    if (since_checked_queue > 1000)
    {
      since_checked_queue = 0;
      actual = QwiicButtonTask::queue->peek<QwiickButtonState>(__func__);
      if (actual != nullptr && actual->pressed)
      {
        found = true;
        Serial.printf("Qwiic button pressed\n");
      }
      else if (actual != nullptr)
        Serial.printf("Press Qwiic button: %d\n", actual->pressed);
      else
        Serial.printf("Press Qwiic button\n");
    }
    vTaskDelay(5);
  }

  TEST_ASSERT_TRUE(actual->pressed);
}

void test_qwiicButton()
{
  uint8_t actual = 1;
  TEST_ASSERT_EQUAL(1, actual);
}

void setup()
{
  delay(2000);

  Wire.begin(); //Join I2C bus

  Serial.begin(115200);

  mutex_I2C.create("i2c", /*default*/ TICKS_5);
  mutex_I2C.enabled = true;

  QwiicButtonTask::createTask(CORE_0, PRIORITY_1);

  while (!QwiicButtonTask::taskReady)
  {
    vTaskDelay(5);
  }

  UNITY_BEGIN();

  RUN_TEST(test_qwiic_queue_sent);

  UNITY_END();
}

void loop()
{
}
