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

// runs every test
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
        last_id = actual->event_id;
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
  NintendoClassicTask::mgr.create(NintendoClassicTask::task, CORE_0, PRIORITY_1);
  Display::mgr.create(Display::task, CORE_0, PRIORITY_1);
  Remote::mgr.create(Remote::task, CORE_0, PRIORITY_1);

  Serial.printf("Waiting for tasks to start\n");
  while (!Display::mgr.ready || !NintendoClassicTask::mgr.ready || !Remote::mgr.ready)
  {
    vTaskDelay(5);
  }
  Serial.printf("Tasks ready\n");

  Serial.printf("Watch then click the button to end the test\n");

  unsigned long last_id = -1;
  BoardClass board;

  while (1)
  {
    if (since_checked_queue > 100)
    {
      since_checked_queue = 0;

      NintendoButtonEvent *btn = NintendoClassicTask::queue->peek<NintendoButtonEvent>(__func__);

      if (btn != nullptr && !btn->been_peeked(last_id))
      {
        last_id = btn->event_id;
        if ((btn->button == NintendoController::BUTTON_RIGHT ||
             btn->button == NintendoController::BUTTON_UP ||
             btn->button == NintendoController::BUTTON_DOWN) &&
            btn->state == NintendoController::BUTTON_PRESSED)
        {
          Serial.printf("Sending to boardPacketQueue\n");
          VescData data;
          if (btn->button == NintendoController::BUTTON_DOWN)
          {
            board.packet.moving = false;
          }
          else
          {
            data.moving = btn->button == NintendoController::BUTTON_UP;
            data.id = last_id;
            data.version = VERSION_BOARD_COMPAT;
            board.save(data);
          }
          board.id++;
          boardPacketQueue->sendLegacy(&board);
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

void test_motor_current()
{
  NintendoClassicTask::mgr.create(NintendoClassicTask::task, CORE_0, PRIORITY_1);
  Display::mgr.create(Display::task, CORE_0, PRIORITY_1);
  Remote::mgr.create(Remote::task, CORE_0, PRIORITY_1);

  Serial.printf("Waiting for tasks to start\n");
  while (!Display::mgr.ready || !NintendoClassicTask::mgr.ready || !Remote::mgr.ready)
  {
    vTaskDelay(5);
  }

  Serial.printf("Tasks ready\n");

  Serial.printf("Watch then click the button to end the test\n");

  VescData data;

  BoardClass board;
  data.moving = true;
  data.version = VERSION_BOARD_COMPAT;
  board.save(data);
  board.id = 1;
  boardPacketQueue->send(&board);

  elapsedMillis since_sent_packet = 0;

  while (since_sent_packet < 1000)
  {
    vTaskDelay(10);
  }
  since_sent_packet = 0;

  data.moving = false;
  board.save(data);
  boardPacketQueue->send(&board);

  unsigned long last_id = -1;

  while (1)
  {
    if (since_sent_packet > 1000)
    {
      TEST_ASSERT_TRUE(Display::fsm_mgr.currentStateIs(Display::StateId::ST_STOPPED_SCREEN));
    }

    if (since_checked_queue > 100)
    {
      since_checked_queue = 0;

      NintendoButtonEvent *btn = NintendoClassicTask::queue->peek<NintendoButtonEvent>(__func__);

      if (btn != nullptr && !btn->been_peeked(last_id))
      {
        last_id = btn->event_id;
        if ((btn->button == NintendoController::BUTTON_RIGHT ||
             btn->button == NintendoController::BUTTON_UP ||
             btn->button == NintendoController::BUTTON_DOWN) &&
            btn->state == NintendoController::BUTTON_PRESSED)
        {
          Serial.printf("Sending to boardPacketQueue\n");
          VescData data;
          if (btn->button == NintendoController::BUTTON_DOWN)
          {
            board.packet.moving = false;
          }
          else
          {
            data.moving = btn->button == NintendoController::BUTTON_UP;
            data.id = last_id;
            data.version = VERSION_BOARD_COMPAT;
            board.save(data);
          }
          boardPacketQueue->send(&board);
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

void test_board_comms()
{
  Display::mgr.create(Display::task, CORE_0, PRIORITY_1);
  BoardCommsTask::mgr.create(BoardCommsTask::task, CORE_1, PRIORITY_4, WITH_HEALTHCHECK);

  Serial.printf("Waiting for tasks to start\n");
  while (!Display::mgr.ready || !BoardCommsTask::mgr.ready)
  {
    vTaskDelay(5);
  }

  Serial.printf("Tasks ready\n");

  elapsedMillis since_mocked_board;

  while (1)
  {
    if (since_checked_queue > 100)
    {
      since_checked_queue = 0;
    }

    if (since_mocked_board > 3000)
    {
      since_mocked_board = 0;
    }

    vTaskDelay(5);
  }
}

void test_board_replies_with_same_id()
{
  // Display::mgr.create(Display::task, CORE_0, PRIORITY_1);
  BoardCommsTask::mgr.create(BoardCommsTask::task, CORE_1, PRIORITY_4);
  SendToBoardTimerTask::mgr.create(SendToBoardTimerTask::task, CORE_1, PRIORITY_3);

  Serial.printf("Waiting for tasks to start\n");

  while (
      // !Display::mgr.ready ||
      !BoardCommsTask::mgr.ready ||
      !SendToBoardTimerTask::mgr.ready)
  {
    vTaskDelay(5);
  }

  Serial.printf("Tasks ready\n");

  const unsigned long TEST_DURATION_IN_SECONDS = 60;
  const unsigned long TEST_DURATION = 1000 * TEST_DURATION_IN_SECONDS;

  Serial.printf("----------------------------------\n");
  Serial.printf("TEST: checking response id matches\n");
  Serial.printf("sent id for %d seconds \n", TEST_DURATION_IN_SECONDS);
  Serial.printf("----------------------------------\n");

  elapsedMillis since_test_started;

  unsigned long last_pkt_id = -1;

  while (since_test_started < TEST_DURATION)
  {
    PacketState *packet = packetStateQueue->peek<PacketState>(__func__);
    if (packet != nullptr && packet->event_id != last_pkt_id)
    {
      last_pkt_id = packet->event_id;

      TEST_ASSERT_TRUE_MESSAGE(packet->connected(), "Either ids don't match or reply has timed out");
    }

    vTaskDelay(10);
  }
}

void test_mocked_client_responds_to_controller_packets_correctly()
{
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

  Serial.printf("Tasks ready\n");

  const unsigned long TEST_DURATION_IN_SECONDS = 60;
  const unsigned long TEST_DURATION = 1000 * TEST_DURATION_IN_SECONDS;

  Serial.printf("----------------------------------\n");
  Serial.printf("TEST: mock response received\n");
  Serial.printf("----------------------------------\n");

  elapsedMillis since_test_started;

  unsigned long last_pkt_id = -1;

  while (since_test_started < TEST_DURATION)
  {
    PacketState *packet = packetStateQueue->peek<PacketState>(__func__);
    if (packet != nullptr && packet->event_id != last_pkt_id)
    {
      last_pkt_id = packet->event_id;

      TEST_ASSERT_TRUE_MESSAGE(packet->connected(), "Either ids don't match or reply has timed out");
    }

    vTaskDelay(10);
  }
}

void test_disp_showing_stopped_when_mocked_board_is_responding_correctly()
{
  Display::mgr.create(Display::task, CORE_0, PRIORITY_1);
  BoardCommsTask::mgr.create(BoardCommsTask::task, CORE_1, PRIORITY_4);
  SendToBoardTimerTask::mgr.create(SendToBoardTimerTask::task, CORE_1, PRIORITY_3);

  // pass in controller_packet
  BoardCommsTask::boardClient.mockResponseCallback([](ControllerData out) {
    VescData mockresp;
    mockresp.id = out.id;
    mockresp.moving = false;
    mockresp.version = VERSION_BOARD_COMPAT;
    return mockresp;
  });

  Serial.printf("Waiting for tasks to start\n");
  while (!Display::mgr.ready ||
         !BoardCommsTask::mgr.ready ||
         !SendToBoardTimerTask::mgr.ready)
  {
    vTaskDelay(5);
  }
  Serial.printf("Tasks ready\n");

  SendToBoardTimerTask::mgr.enable();
  BoardCommsTask::mgr.enable();

  elapsedMillis
      since_sent_to_board,
      since_check_board_queue;

  unsigned long last_pkt_ev_id = -1;

  Serial.printf("----------------------------------------------------------------\n");
  Serial.printf("TEST: checking display showing 'Stopped' when  board responding \n");
  Serial.printf("----------------------------------------------------------------\n");

  while (1)
  {
    if (since_check_board_queue > PERIOD_50MS)
    {
      since_check_board_queue = 0;

      PacketState *packet = packetStateQueue->peek<PacketState>(__func__);
      if (packet != nullptr && packet->event_id != last_pkt_ev_id)
      {
        last_pkt_ev_id = packet->event_id;

        if (packet->connected())
        {
          if (packet->acknowledged())
          {
            Serial.printf("Acknowledged packet_id %lu\n", packet->packet_id);
            if (packet->packet_id == 4)
            {
              bool stopped_screen = Display::_fsm.getCurrentStateId() == Display::ST_STOPPED_SCREEN;
              TEST_ASSERT_TRUE_MESSAGE(stopped_screen, "Display not showing STOPPED_SCREEN");
              break;
            }
          }
          bool connected = packet->packet_id > 0 && packet->connected();
          TEST_ASSERT_TRUE_MESSAGE(connected, "(Mock)Board not connected");
        }
      }
    }
    vTaskDelay(5);
  } //end while
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
  // RUN_TEST(test_display_remote_battery);
  // RUN_TEST(test_motor_current);
  // RUN_TEST(test_board_comms);
  // RUN_TEST(test_board_replies_with_same_id);
  // RUN_TEST(test_disp_showing_stopped_when_board_responding);
  // RUN_TEST(test_mocked_client_responds_to_controller_packets_correctly);
  RUN_TEST(test_disp_showing_stopped_when_mocked_board_is_responding_correctly);

  UNITY_END();
}

void loop()
{
}
