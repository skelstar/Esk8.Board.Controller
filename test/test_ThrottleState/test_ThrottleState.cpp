#include <Arduino.h>
#include <unity.h>

#define DEBUG_SERIAL 1

#ifdef DEBUG_SERIAL
#define DEBUG_OUT Serial
#endif
#define PRINTSTREAM_FALLBACK
#include "Debug.hpp"

#define PRINT_MUTEX_TAKE_FAIL 1
static int counter = 0;

#include <types.h>
#include <rtosManager.h>
#include <QueueManager.h>
#include <elapsedMillis.h>
#include <RTOSTaskManager.h>
#include <BoardClass.h>

#include <MockMagThrottle.h>
#include <MockedQwiicButton.h>

MyMutex mutex_I2C;
MyMutex mutex_SPI;

xQueueHandle xBoardPacketQueue;
Queue::Manager *boardPacketQueue = nullptr;

xQueueHandle xSendToBoardQueueHandle;
Queue::Manager *sendToBoardQueue = nullptr;

xQueueHandle xBoardStateQueueHandle;
Queue::Manager *packetStateQueue = nullptr;

xQueueHandle xPrimaryButtonQueue;
Queue::Manager *primaryButtonQueue = nullptr;

xQueueHandle xStatsQueue;
Queue::Manager *statsQueue = nullptr;

xQueueHandle xNintendoControllerQueue;
Queue::Manager *nintendoControllerQueue = nullptr;

xQueueHandle xThrottleQueue;
Queue::Manager *throttleQueue = nullptr;

class SendToBoardNotf : public QueueBase
{
public:
  const char *name;
};

#include <tasks/core0/SendToBoardTimerTask.h>

//----------------------------------
#define PRINT_TASK_STARTED_FORMAT "TASK: %s on Core %d\n"
#define PRINT_THROTTLE 0

#define TICKS_5ms 5 / portTICK_PERIOD_MS
#define TICKS_10ms 10 / portTICK_PERIOD_MS
#define TICKS_50ms 50 / portTICK_PERIOD_MS
#define TICKS_100ms 100 / portTICK_PERIOD_MS

#include <tasks/core0/remoteTask.h>

#include <displayState.h>

#include <tasks/core0/qwiicButtonTask.h>
// #include <tasks/core0/NintendoClassicTask.h>
#include <tasks/core0/ThrottleTask.h>
// #include <tasks/core0/debugTask.h>
// #include <tasks/core0/displayTask.h>

elapsedMillis since_checked_queue;

const unsigned long SECONDS = 1000;

void printTestTitle(const char *name)
{
  Serial.printf("-------------------------------------------\n");
  Serial.printf("  TEST: %s\n", name);
  Serial.printf("-------------------------------------------\n");
}

void printTestInstructions(const char *instructions)
{
  Serial.printf("*** INSTR: %s\n", instructions);
}

void setUp()
{
  mutex_I2C.create("i2c", /*default*/ TICKS_5ms);
  mutex_I2C.enabled = true;

  xPrimaryButtonQueue = xQueueCreate(1, sizeof(PrimaryButtonState));
  primaryButtonQueue = new Queue::Manager(xPrimaryButtonQueue, (TickType_t)5);

  xThrottleQueue = xQueueCreate(1, sizeof(ThrottleState));
  throttleQueue = new Queue::Manager(xThrottleQueue, (TickType_t)5);
}

void tearDown()
{
}

#define CORE_0 0
#define PRIORITY_1 1

void test_throttle_state_queue()
{
  namespace throttle_ = ThrottleTask;

  unsigned long _last_throttle_queue_id = 0;

  Wire.begin(); //Join I2C bus

  throttle_::mgr.create(throttle_::task, CORE_0, PRIORITY_1);

  static uint8_t _throttle = 127;

  MagneticThrottle::setGetThrottleCb([] {
    _throttle = _throttle - 1;
    return _throttle;
  });

  while (!throttle_::mgr.ready)
  {
    vTaskDelay(5);
  }

  printTestInstructions("Will test the throttle state queue");

  elapsedMillis since_checked_queue;

  PrimaryButtonState state;

  counter = 0;
  char message[40];

  while (counter < 6)
  {
    if (since_checked_queue > 1000)
    {
      since_checked_queue = 0;

      state.pressed = false;

      primaryButtonQueue->send(&state);

      vTaskDelay(50);

      ThrottleState *throttle = throttleQueue->peek<ThrottleState>(__func__);
      if (throttle != nullptr && !throttle->been_peeked(_last_throttle_queue_id))
      {
        _last_throttle_queue_id = throttle->event_id;
        Serial.printf("Got %d from ThrottleTask\n", throttle->val);
        TEST_ASSERT_EQUAL(_throttle, throttle->val);
      }
      else
      {
        Serial.printf("ThrottleTask didn't respond\n");
      }
      counter++;
    }
    vTaskDelay(5);
  }
}

void test_throttle_limting_with_primary_button()
{
  namespace throttle_ = ThrottleTask;
  namespace qwiic_ = QwiicButtonTask;

  unsigned long _last_throttle_queue_id = 0;

  Wire.begin(); //Join I2C bus

  throttle_::mgr.create(throttle_::task, CORE_0, PRIORITY_1);
  qwiic_::mgr.create(QwiicButtonTask::task, CORE_0, PRIORITY_1);

  static uint8_t _s_Throttle = 127;
  static uint8_t _s_Qwiic_pressed = 0;

  qwiic_::qwiicButton.setMockIsPressedCallback([] {
    _s_Qwiic_pressed = counter >= 3;
    return _s_Qwiic_pressed == 1;
  });

  static uint8_t _s_Steps[] = {127, 140, 130, 127, 140, 130};

  MagneticThrottle::setGetThrottleCb([] {
    _s_Throttle = _s_Steps[counter];
    return _s_Throttle;
  });

  while (!throttle_::mgr.ready && !qwiic_::mgr.ready)
  {
    vTaskDelay(5);
  }

  printTestInstructions("Will test the throttle limiting (by primaryButton)");

  elapsedMillis since_checked_queue;

  PrimaryButtonState state;

  counter = 0;
  char message[40];

  while (counter < 6)
  {
    if (since_checked_queue > 1000)
    {
      since_checked_queue = 0;

      state.pressed = _s_Qwiic_pressed;

      primaryButtonQueue->send(&state);

      vTaskDelay(50);

      ThrottleState *throttle = throttleQueue->peek<ThrottleState>(__func__);
      if (throttle != nullptr && !throttle->been_peeked(_last_throttle_queue_id))
      {
        _last_throttle_queue_id = throttle->event_id;
        Serial.printf("Got %d from ThrottleTask\n", throttle->val);
        if (_s_Qwiic_pressed == 0)
          TEST_ASSERT_EQUAL(throttle->val, 127);
        else
          TEST_ASSERT_EQUAL(throttle->val, _s_Steps[counter]);
      }
      else
      {
        Serial.printf("ThrottleTask queue was empty\n");
      }
      counter++;
    }
    vTaskDelay(5);
  }
}

void setup()
{
  delay(2000);

  Serial.begin(115200);

  UNITY_BEGIN();

  // RUN_TEST(test_throttle_state_queue);
  RUN_TEST(test_throttle_limting_with_primary_button);

  UNITY_END();
}

void loop()
{
}
