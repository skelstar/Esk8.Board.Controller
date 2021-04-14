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
#include <MockNintendoController.h>
#include <elapsedMillis.h>
#include <RTOSTaskManager.h>
#include <BoardClass.h>

MyMutex mutex_I2C;
MyMutex mutex_SPI;

xQueueHandle xBoardPacketQueue;
Queue::Manager *boardPacketQueue = nullptr;

xQueueHandle xSendToBoardQueueHandle;
Queue::Manager *sendToBoardQueue = nullptr;

xQueueHandle xPacketStateQueueHandle;
Queue::Manager *packetStateQueue = nullptr;

xQueueHandle xPrimaryButtonQueueHandle;
Queue::Manager *primaryButtonQueue = nullptr;

xQueueHandle xStatsQueue;
Queue::Manager *statsQueue = nullptr;

xQueueHandle xNintendoControllerQueue;
Queue::Manager *nintendoControllerQueue = nullptr;

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

// #include <tasks/core0/QwiicButtonTask.h>
#include <tasks/core0/NintendoClassicTask.h>
// #include <tasks/core0/ThrottleTask.h>
// #include <tasks/core0/debugTask.h>
// #include <tasks/core0/displayTask.h>

elapsedMillis since_checked_queue;

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

  xNintendoControllerQueue = xQueueCreate(1, sizeof(NintendoButtonEvent *));
  nintendoControllerQueue = new Queue::Manager(xNintendoControllerQueue, TICKS_5ms);
}

void tearDown()
{
}

void test_qwiic_button_pressed_then_released_via_queue()
{
  namespace ninten_ = NintendoClassicTask;

  unsigned long _last_queue_id = 0;

  Wire.begin(); //Join I2C bus

  ninten_::mgr.create(ninten_::task, CORE_0, PRIORITY_1);

  ninten_::classic.setMockGetButtonEventCallback([] {
    Serial.printf("counter: %d\n", counter);
    switch (counter)
    {
    case 0:
      Serial.printf("picking: %s\n", "BUTTON_LEFT");
      return (uint8_t)NintendoController::BUTTON_LEFT;
    case 1:
      Serial.printf("picking: %s\n", "BUTTON_RIGHT");
      return (uint8_t)NintendoController::BUTTON_RIGHT;
    case 2:
      Serial.printf("picking: %s\n", "BUTTON_UP");
      return (uint8_t)NintendoController::BUTTON_UP;
    case 3:
      Serial.printf("picking: %s\n", "BUTTON_START");
      return (uint8_t)NintendoController::BUTTON_START;
    case 4:
      Serial.printf("picking: %s\n", "BUTTON_DOWN");
      return (uint8_t)NintendoController::BUTTON_DOWN;
    case 5:
      Serial.printf("picking: %s\n", "BUTTON_SELECT");
      return (uint8_t)NintendoController::BUTTON_SELECT;
    }
    return (uint8_t)NintendoController::BUTTON_A;
  });

  while (!ninten_::mgr.ready)
  {
    vTaskDelay(5);
  }

  printTestInstructions("Will cycle through 5 buttons: LEFT, RIGHT, UP, DOWN, START, SELECT");

  elapsedMillis since_checked_queue;

  while (counter < 6)
  {
    if (since_checked_queue > 500)
    {
      since_checked_queue = 0;

      NintendoButtonEvent *state = nintendoControllerQueue->peek<NintendoButtonEvent>(__func__);
      if (state != nullptr && !state->been_peeked(_last_queue_id))
      {
        _last_queue_id = state->event_id;
        Serial.printf("counter: %lu and button pressed: %s\n", counter, NintendoController::getButton(state->button));
        // TEST_ASSERT_EQUAL(counter % 2 == 0, state->pressed);
        counter++;
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

  RUN_TEST(test_qwiic_button_pressed_then_released_via_queue);

  UNITY_END();
}

void loop()
{
}
