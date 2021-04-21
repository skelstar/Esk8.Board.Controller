#pragma once

// #include <tasks/root.h>
// #include <tasks/queues/types/root.h>
#include <QueueManager1.h>

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
} // end namespace