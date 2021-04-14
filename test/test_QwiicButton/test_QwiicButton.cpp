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
#include <MockedQwiicButton.h>
#include <elapsedMillis.h>
#include <RTOSTaskManager.h>
#include <BoardClass.h>

MyMutex mutex_I2C;
MyMutex mutex_SPI;

xQueueHandle xBoardPacketQueue;
Queue::Manager *boardPacketQueue = nullptr;

// xQueueHandle xSendToBoardQueueHandle;

xQueueHandle xPacketStateQueueHandle;
Queue::Manager *packetStateQueue = nullptr;

xQueueHandle xPrimaryButtonQueueHandle;
Queue::Manager *primaryButtonQueue = nullptr;

xQueueHandle xStatsQueue;
Queue::Manager *statsQueue = nullptr;

class SendToBoardNotf : public QueueBase
{
public:
  const char *name;
};

//----------------------------------
#include <RF24Network.h>
#include <NRF24L01Lib.h>
#include <MockGenericClient.h>

NRF24L01Lib nrf24;

RF24 radio(NRF_CE, NRF_CS);
RF24Network network(radio);

#include <tasks/core0/BoardCommsTask.h>
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

#include <tasks/core0/QwiicButtonTask.h>
// #include <tasks/core0/NintendoClassicTask.h>
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

PrimaryButtonState mockQwiicButtonStateResponse(ControllerData out)
{
  PrimaryButtonState state;
  state.pressed = false;
  return state;
}

void setUp()
{
  mutex_I2C.create("i2c", /*default*/ TICKS_5ms);
  mutex_I2C.enabled = true;

  xPrimaryButtonQueueHandle = xQueueCreate(1, sizeof(PrimaryButtonState *));
  primaryButtonQueue = new Queue::Manager(xPrimaryButtonQueueHandle, TICKS_5ms);
  // Queue1::Manager<SendToBoardNotf> sendToBoardQueue(xSendToBoardQueueHandle, TICKS_5ms);
}

void tearDown()
{
}

#define CORE_0 0
#define PRIORITY_1 1

void test_qwiic_button_pressed_then_released_via_queue()
{
  namespace qwiic_ = QwiicButtonTask;

  unsigned long _last_queue_id = 0;

  Wire.begin(); //Join I2C bus

  qwiic_::mgr.create(QwiicButtonTask::task, CORE_0, PRIORITY_1);

  qwiic_::qwiicButton.setMockIsPressedCallback([] {
    return counter % 2 == 0;
  });

  while (!QwiicButtonTask::mgr.ready)
  {
    vTaskDelay(5);
  }

  printTestInstructions("Will test primary button queue with qwiic mock button");

  elapsedMillis since_checked_queue;

  while (counter < 6)
  {
    if (since_checked_queue > 500)
    {
      since_checked_queue = 0;

      PrimaryButtonState *state = primaryButtonQueue->peek<PrimaryButtonState>(__func__);
      if (state != nullptr && !state->been_peeked(_last_queue_id))
      {
        _last_queue_id = state->event_id;
        Serial.printf("counter: %lu and pressed: %s\n", counter, state->pressed ? "PRESSED" : "RELEASED");
        TEST_ASSERT_EQUAL(counter % 2 == 0, state->pressed);
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

  RUN_TEST(test_qwiic_button_pressed_then_released_via_queue);

  UNITY_END();
}

void loop()
{
}
