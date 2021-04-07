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

xQueueHandle xSendToBoardQueueHandle;
Queue::Manager *sendToBoardQueue;

xQueueHandle xBoardStateQueueHandle;
Queue::Manager *packetStateQueue;

xQueueHandle xPrimaryButtonQueue;
Queue::Manager *primaryButtonQueue = nullptr;

class SendToBoardNotf : public QueueBase
{
public:
  const char *name;
};

xQueueHandle xStatsQueue;
Queue::Manager *statsQueue;

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

VescData mockStoppedResponse(ControllerData out)
{
  VescData mockresp;
  mockresp.id = out.id;
  mockresp.version = VERSION_BOARD_COMPAT;
  mockresp.moving = false;
  return mockresp;
}

void setUp()
{
  nrf24.begin(&radio, &network, COMMS_CONTROLLER);

  mutex_SPI.create("SPI", /*default*/ TICKS_50ms);
  mutex_I2C.create("i2c", /*default*/ TICKS_5ms);
  mutex_I2C.enabled = true;

  Wire.begin(); //Join I2C bus
  // Wire.setClock(200000);

  xBoardPacketQueue = xQueueCreate(1, sizeof(BoardClass *));
  boardPacketQueue = new Queue::Manager(xBoardPacketQueue, (TickType_t)5);

  xStatsQueue = xQueueCreate(1, sizeof(StatsClass *));
  statsQueue = new Queue::Manager(xStatsQueue, (TickType_t)5);

  xSendToBoardQueueHandle = xQueueCreate(1, sizeof(SendToBoardNotf *));
  sendToBoardQueue = new Queue::Manager(xSendToBoardQueueHandle, (TickType_t)5);

  xBoardStateQueueHandle = xQueueCreate(1, sizeof(PacketState *));
  packetStateQueue = new Queue::Manager(xBoardStateQueueHandle, (TickType_t)5);

  xPrimaryButtonQueue = xQueueCreate(1, sizeof(PrimaryButtonState));
  primaryButtonQueue = new Queue::Manager(xPrimaryButtonQueue, (TickType_t)5);
}

void tearDown()
{
}

void test_qwiic_button_pressed_then_released_via_queue()
{
  Wire.begin(); //Join I2C bus

  PrimaryButtonState *actual;

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
      actual = primaryButtonQueue->peek<PrimaryButtonState>(__func__);
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
        last_id = actual->event_id;
      }
    }

    vTaskDelay(5);
  }

  TEST_ASSERT_TRUE(throttle > 220);
}

void test_display_remote_battery()
{
  printTestTitle(__func__);

  NintendoClassicTask::mgr.create(NintendoClassicTask::task, CORE_0, PRIORITY_1);
  Display::mgr.create(Display::task, CORE_0, PRIORITY_1);
  Remote::mgr.create(Remote::task, CORE_0, PRIORITY_1);
  BoardCommsTask::mgr.create(BoardCommsTask::task, CORE_1, PRIORITY_3);
  SendToBoardTimerTask::mgr.create(SendToBoardTimerTask::task, CORE_1, PRIORITY_3);

  BoardCommsTask::boardClient.mockResponseCallback(mockStoppedResponse);
  BoardCommsTask::mgr.enable();

  Serial.printf("Waiting for tasks to start\n");
  while (!Display::mgr.ready || !NintendoClassicTask::mgr.ready || !Remote::mgr.ready)
  {
    vTaskDelay(5);
  }
  Serial.printf("Tasks ready\n");

  printTestInstructions("Watch then pressed START to end the test\n");

  unsigned long last_id = -1;
  BoardClass board;

  SendToBoardTimerTask::mgr.enable();

  while (1)
  {
    if (since_checked_queue > 100)
    {
      since_checked_queue = 0;

      NintendoButtonEvent *btn = NintendoClassicTask::queue->peek<NintendoButtonEvent>(__func__);

      if (btn != nullptr && !btn->been_peeked(last_id))
      {
        last_id = btn->event_id;
        if (btn != nullptr && btn->button == NintendoController::BUTTON_START)
        {
          TEST_ASSERT_TRUE(true);
        }
      }
    }

    vTaskDelay(5);
  }
}

void test_mocked_client_responds_to_controller_packets_correctly()
{
  printTestTitle(__func__);

  BoardCommsTask::mgr.create(BoardCommsTask::task, CORE_1, PRIORITY_4);
  SendToBoardTimerTask::mgr.create(SendToBoardTimerTask::task, CORE_1, PRIORITY_3);

  // pass in controller_packet
  BoardCommsTask::boardClient.mockResponseCallback([](ControllerData out) {
    VescData mockresp;
    mockresp.id = out.id;
    return mockresp;
  });

  Serial.printf("Waiting for tasks to start\n");

  while (
      !BoardCommsTask::mgr.ready ||
      !SendToBoardTimerTask::mgr.ready)
  {
    vTaskDelay(5);
  }

  SendToBoardTimerTask::setInterval(1000);

  BoardCommsTask::mgr.enable();
  SendToBoardTimerTask::mgr.enable();

  Serial.printf("Tasks ready\n");

  const unsigned long TEST_DURATION = 60 * SECONDS;

  elapsedMillis since_test_started;

  unsigned long last_ev_id = -1, last_pkt_id = -1;

  char instructions[60];
  sprintf(instructions, "Testing mocked board connected for %lu seconds", TEST_DURATION / SECONDS);
  printTestInstructions(instructions);

  while (since_test_started < TEST_DURATION)
  {
    PacketState *packet = packetStateQueue->peek<PacketState>(__func__);
    if (packet != nullptr && packet->event_id != last_ev_id)
    {
      last_ev_id = packet->event_id;
      if (packet->packet_id != last_pkt_id)
      {
        last_pkt_id = packet->packet_id;
        Serial.printf("Mocked board responded: %lu\n", packet->packet_id);
      }

      TEST_ASSERT_TRUE_MESSAGE(packet->connected(), "Either ids don't match or reply has timed out");
    }

    vTaskDelay(10);
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
  // RUN_TEST(test_display_remote_battery);
  RUN_TEST(test_mocked_client_responds_to_controller_packets_correctly);

  UNITY_END();
}

void loop()
{
}
