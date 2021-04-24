#include <tasks/core0/BoardCommsTask.h>
#include <tasks/core0/DisplayTaskBase.h>
#include <tasks/core0/NintendoClassicTaskBase.h>
#include <tasks/core0/QwiicTaskBase.h>
#include <tasks/core0/ThrottleTaskBase.h>

#include <tasks/core0/RemoteTask.h>
RemoteTask remoteTask;

namespace Remote
{
  void task1(void *parameters)
  {
    remoteTask.task(parameters);
  }
}
