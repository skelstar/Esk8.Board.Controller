#include <Arduino.h>
#include <unity.h>

#define DEBUG_SERIAL 1

#ifdef DEBUG_SERIAL
#define DEBUG_OUT Serial
#endif
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#define PRINT_MUTEX_TAKE_FAIL 1

#include <types.h>
#include <rtosManager.h>
#include <QueueManager.h>
#include <SparkFun_Qwiic_Button.h>
#include <elapsedMillis.h>
#include <RTOSTaskManager.h>
#include <BoardClass.h>

MyMutex mutex_I2C;
MyMutex mutex_SPI;

xQueueHandle xBoardPacketQueue;
Queue::Manager *boardPacketQueue;

xQueueHandle queueHandle;
Queue::Manager *queue;

xQueueHandle xStatsQueue;
Queue::Manager *statsQueue;

#define PRINT_TASK_STARTED_FORMAT "TASK: %s on Core %d\n"
#define PRINT_THROTTLE 0

#define TICKS_5ms 5 / portTICK_PERIOD_MS
#define TICKS_10ms 10 / portTICK_PERIOD_MS
#define TICKS_50ms 50 / portTICK_PERIOD_MS
#define TICKS_100ms 100 / portTICK_PERIOD_MS

#include <tasks/core0/remoteTask.h>

#include <displayState.h>

#include <tasks/core0/qwiicButtonTask.h>
#include <tasks/core0/NintendoClassicTask.h>
#include <tasks/core0/ThrottleTask.h>
#include <tasks/core0/debugTask.h>
#include <tasks/core0/displayTask.h>

#define CORE_0 0
#define PRIORITY_0 0
#define PRIORITY_1 1
#define PRIORITY_2 2
#define PRIORITY_3 3
#define PRIORITY_4 4

elapsedMillis since_checked_queue;

// runs every test
void setUp()
{
  mutex_SPI.create("SPI", /*default*/ TICKS_50ms);
  mutex_I2C.create("i2c", /*default*/ TICKS_5ms);
  mutex_I2C.enabled = true;
}

// runs every test
void tearDown()
{
}

void test_qwiic_button_pressed_then_released_via_queue()
{
  Wire.begin(); //Join I2C bus

  QwiicButtonState *actual;

  QwiicButtonTask::mgr.create(QwiicButtonTask::task, CORE_0, PRIORITY_1);

  while (!QwiicButtonTask::mgr.ready)
  {
    vTaskDelay(5);
  }

  Serial.printf("Press then release the Qwiic button to satify test\n");

  bool was_pressed = false, was_released = false;
  while (!(was_pressed && was_released))
  {
    if (since_checked_queue > 200)
    {
      since_checked_queue = 0;
      actual = QwiicButtonTask::queue->peek<QwiicButtonState>(__func__);
      if (!was_pressed && actual != nullptr && actual->pressed)
      {
        Serial.printf("Qwiic button pressed\n");
        was_pressed = true;
      }
      else if (was_pressed && actual != nullptr && !actual->pressed)
      {
        Serial.printf("Qwiic button released\n");
        was_released = true;
      }
    }
    vTaskDelay(5);
  }
  TEST_ASSERT_TRUE(was_pressed && was_released);
}

void test_nintendo_button_is_pressed_then_released_task()
{
  Wire.begin(); //Join I2C bus
  Wire.setClock(200000);

  NintendoButtonEvent *actual;

  NintendoClassicTask::mgr.create(NintendoClassicTask::task, CORE_0, PRIORITY_1);

  while (!NintendoClassicTask::mgr.ready)
  {
    vTaskDelay(5);
  }

  Serial.printf("Press a button to satify test\n");

  bool was_pressed = false, was_released = false;

  while (!(was_pressed && was_released))
  {
    if (since_checked_queue > 200)
    {
      since_checked_queue = 0;
      actual = NintendoClassicTask::queue->peek<NintendoButtonEvent>(__func__);
      if (!was_pressed && actual != nullptr && actual->state == 1)
      {
        Serial.printf("Nintendo button pressed %d %d\n", actual->button, actual->state);
        was_pressed = true;
      }
      else if (was_pressed && actual != nullptr && actual->state == 0)
      {
        Serial.printf("Nintendo button released\n");
        was_released = true;
      }
    }
    vTaskDelay(5);
  }

  TEST_ASSERT_TRUE(was_pressed && was_released);
}

void test_magnetic_throttle_is_moved_greater_than_220()
{
  Wire.begin(); //Join I2C bus

  QwiicButtonTask::mgr.create(QwiicButtonTask::task, CORE_0, PRIORITY_1);
  ThrottleTask::mgr.create(ThrottleTask::task, CORE_0, PRIORITY_1);

  while (!ThrottleTask::mgr.ready)
  {
    vTaskDelay(5);
  }

  Serial.printf("Move throttle to >220 degrees to satify test\n");

  uint8_t throttle = 127;
  unsigned long last_id = 0;

  while (throttle < 220)
  {
    if (since_checked_queue > 200)
    {
      since_checked_queue = 0;

      ThrottleState *actual = ThrottleTask::queue->peek<ThrottleState>(__func__);

      if (actual != nullptr && !actual->been_peeked(last_id) && throttle != actual->val)
      {
        Serial.printf("Throttle changed %d\n", actual->val);
        throttle = actual->val;
        last_id = actual->id;
      }
    }

    vTaskDelay(5);
  }

  TEST_ASSERT_TRUE(throttle > 220);
}

void test_run_debug_task()
{
  Wire.begin(); //Join I2C bus

  QwiicButtonTask::mgr.create(QwiicButtonTask::task, CORE_0, PRIORITY_1);
  ThrottleTask::mgr.create(ThrottleTask::task, CORE_0, PRIORITY_1);
  NintendoClassicTask::mgr.create(NintendoClassicTask::task, CORE_0, PRIORITY_1);
  Debug::mgr.create(Debug::task, CORE_0, PRIORITY_1);

  while (!ThrottleTask::mgr.ready || !Debug::mgr.ready)
  {
    vTaskDelay(5);
  }

  Serial.printf("Watch then click the button to end the test\n");

  while (1)
  {
    if (since_checked_queue > 200)
    {
      since_checked_queue = 0;

      QwiicButtonState *button = QwiicButtonTask::queue->peek<QwiicButtonState>(__func__);

      if (button != nullptr && button->pressed)
      {
        TEST_ASSERT_TRUE(button->pressed);
      }
    }

    vTaskDelay(5);
  }
}

void test_run_all_tasks()
{
  Wire.begin(); //Join I2C bus
  // Wire.begin(200000); //Join I2C bus

  QwiicButtonTask::mgr.create(QwiicButtonTask::task, CORE_0, PRIORITY_1);
  ThrottleTask::mgr.create(ThrottleTask::task, CORE_0, PRIORITY_1);
  NintendoClassicTask::mgr.create(NintendoClassicTask::task, CORE_0, PRIORITY_1);
  Debug::mgr.create(Debug::task, CORE_0, PRIORITY_1);

  while (!ThrottleTask::mgr.ready || !Debug::mgr.ready)
  {
    vTaskDelay(5);
  }

  Serial.printf("Watch then click the button to end the test\n");

  while (1)
  {
    if (since_checked_queue > 200)
    {
      since_checked_queue = 0;

      QwiicButtonState *button = QwiicButtonTask::queue->peek<QwiicButtonState>(__func__);

      if (button != nullptr && button->pressed)
      {
        TEST_ASSERT_TRUE(button->pressed);
      }
    }

    vTaskDelay(5);
  }
}

void test_display_remote_battery()
{
  Wire.begin(); //Join I2C bus
  Wire.setClock(200000);

  xBoardPacketQueue = xQueueCreate(1, sizeof(BoardClass *));
  boardPacketQueue = new Queue::Manager(xBoardPacketQueue, (TickType_t)5);

  xStatsQueue = xQueueCreate(1, sizeof(StatsClass *));
  statsQueue = new Queue::Manager(xStatsQueue, (TickType_t)5);

  NintendoClassicTask::mgr.create(NintendoClassicTask::task, CORE_0, PRIORITY_1, WITH_HEALTHCHECK);
  Display::mgr.create(Display::task, CORE_0, PRIORITY_1, WITH_HEALTHCHECK);
  Remote::mgr.create(Remote::task, CORE_0, PRIORITY_1, WITH_HEALTHCHECK);

  Serial.printf("Waiting for tasks to start\n");
  while (!Display::mgr.ready || !NintendoClassicTask::mgr.ready || !Remote::mgr.ready)
  {
    vTaskDelay(5);
  }
  Serial.printf("Tasks ready\n");

  Serial.printf("Watch then click the button to end the test\n");

  unsigned long last_id = -1;

  while (1)
  {
    if (since_checked_queue > 200)
    {
      since_checked_queue = 0;

      NintendoButtonEvent *btn = NintendoClassicTask::queue->peek<NintendoButtonEvent>(__func__);

      if (btn != nullptr && !btn->been_peeked(last_id))
      {
        last_id = btn->id;
        if (btn->button == NintendoController::BUTTON_RIGHT && btn->state == NintendoController::BUTTON_PRESSED)
        {
          vTaskDelay(100);
        }
      }

      if (btn != nullptr && btn->button == NintendoController::BUTTON_START)
      {
        TEST_ASSERT_TRUE(btn->state == NintendoController::BUTTON_PRESSED);
      }
    }

    vTaskDelay(5);
  }
}

void setup()
{
  delay(2000);

  Serial.begin(115200);

  UNITY_BEGIN();

  // RUN_TEST(test_qwiic_button_pressed_then_released_via_queue);
  // RUN_TEST(test_nintendo_button_is_pressed_then_released_task);
  // RUN_TEST(test_magnetic_throttle_is_moved_greater_than_220);
  // RUN_TEST(test_run_debug_task);
  // RUN_TEST(test_run_all_tasks);
  RUN_TEST(test_display_remote_battery);

  UNITY_END();
}

void loop()
{
}
