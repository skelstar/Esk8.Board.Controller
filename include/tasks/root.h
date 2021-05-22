#include <tasks/core0/BoardCommsTask.h>
#include <tasks/core0/DisplayTask.h>
#if REMOTE_USED == NINTENDO_REMOTE
#include <tasks/core0/NintendoClassicTask.h>
#include <tasks/core0/QwiicTaskBase.h>
#include <tasks/core0/HapticTask.h>
#elif REMOTE_USED == RED_REMOTE
#include <tasks/core0/DigitalPrimaryButtonTask.h>
#endif
#include <tasks/core0/RemoteTask.h>
#include <tasks/core0/ThrottleTask.h>

#define NUM_TASKS 20