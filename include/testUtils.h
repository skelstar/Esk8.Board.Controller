#pragma once

// #include <tasks/root.h>
// #include <tasks/queues/types/root.h>
#include <QueueManager.h>
#include <tasks/queues/QueueFactory.h>

// #define ASSERT(condition, message)                                                        \
//   do                                                                                      \
//   {                                                                                       \
//     if (!(condition))                                                                     \
//     {                                                                                     \
//       Serial.printf("Assertion: %s | file: %s line: %s \n", message, __FILE__, __LINE__); \
//     }                                                                                     \
//   } while (false);

namespace Test
{
  void waitForTasksReady()
  {
    while (
        BoardCommsTask::thisTask->ready == false ||
        NintendoClassicTaskBase::thisTask->ready == false ||
        QwiicTaskBase::thisTask->ready == false ||
        DisplayTaskBase::thisTask->ready == false ||
        ThrottleTaskBase::thisTask->ready == false)
      vTaskDelay(PERIOD_10ms);
  }

  void enableAllTasks(bool print = false)
  {
    BoardCommsTask::thisTask->enable(print);
    NintendoClassicTaskBase::thisTask->enable(print);
    QwiicTaskBase::thisTask->enable(print);
    DisplayTaskBase::thisTask->enable(print);
    ThrottleTaskBase::thisTask->enable(print);
  }

  void printLine(const char *start, int dashes = 10)
  {
    Serial.print(start);
    int i = 0;
    while (i++ < dashes)
      Serial.print("-");
    Serial.println();
  }

  void printTestInstructions(const char *instructions)
  {
    vTaskDelay(TICKS_500ms);
    const char *st = "*** ";
    printLine(st, strlen(instructions) + 7);
    Serial.printf("*** INSTR: %s\n", instructions);
    printLine(st, strlen(instructions) + 7);
    vTaskDelay(TICKS_500ms);
  }

  void printTestTitle(const char *name)
  {
    vTaskDelay(TICKS_500ms);
    const char *st = "*** ";
    printLine(st, strlen(name) + 7);
    Serial.printf("*** TEST: %s\n", name);
    printLine(st, strlen(name) + 7);
    vTaskDelay(TICKS_500ms);
  }

  void printPASS(const char *message)
  {
    Serial.printf("[PASS @%lums] %s\n", millis(), message);
  }

  void setupAllTheTasks(const char *file = "File not provided")
  {
    DEBUG("----------------------------");
    Serial.printf("    %s \n", file);
    DEBUG("----------------------------");

    xDisplayQueueHandle = xQueueCreate(1, sizeof(DisplayEvent *));
    xNintendoControllerQueue = xQueueCreate(1, sizeof(NintendoButtonEvent *));
    xPacketStateQueueHandle = xQueueCreate(1, sizeof(PacketState *));
    xPrimaryButtonQueueHandle = xQueueCreate(1, sizeof(PrimaryButtonState *));
    xThrottleQueueHandle = xQueueCreate(1, sizeof(ThrottleState *));

    // configure queues
    displayEventQueue = createQueue<DisplayEvent>("(test)displayEventQueue");
    primaryButtonQueue = createQueue<PrimaryButtonState>("(test)primaryButtonQueue");
    packetStateQueue = createQueue<PacketState>("(test)packetStateQueue");
    nintendoQueue = createQueue<NintendoButtonEvent>("(test)nintendoQueue");
    throttleQueue = createQueue<ThrottleState>("(test)throttleQueue");

    QwiicTaskBase::start(TASK_PRIORITY_1, /*work*/ PERIOD_100ms);
    BoardCommsTask::start(TASK_PRIORITY_1, /*work*/ PERIOD_100ms, /*send*/ PERIOD_200ms);
    NintendoClassicTaskBase::start(TASK_PRIORITY_1, /*work*/ PERIOD_50ms);
    DisplayTaskBase::start(TASK_PRIORITY_1, /*work*/ PERIOD_50ms);
    ThrottleTaskBase::start(TASK_PRIORITY_1, /*work*/ PERIOD_200ms);
  }

  void tearDownAllTheTasks()
  {
    QwiicTaskBase::thisTask->deleteTask(PRINT_THIS);
    BoardCommsTask::thisTask->deleteTask(PRINT_THIS);
    NintendoClassicTaskBase::thisTask->deleteTask(PRINT_THIS);
    DisplayTaskBase::thisTask->deleteTask(PRINT_THIS);
    ThrottleTaskBase::thisTask->deleteTask(PRINT_THIS);
  }

  VescData mockBoardStoppedResponse(ControllerData out)
  {
    VescData mockresp;
    mockresp.id = out.id;
    mockresp.version = VERSION_BOARD_COMPAT;
    mockresp.moving = false;
    // Serial.printf("[%lu] mockMovingResponse called, moving: %d\n", millis(), mockresp.moving);
    return mockresp;
  }
} // end namespace