#include <tasks/core0/BoardCommsTask.h>
#include <tasks/core0/ThrottleTask.h>

#ifdef USE_REMOTE_TASK
#include <tasks/core0/RemoteTask.h>
#endif
#ifdef USE_TDISPLAY
#include <tasks/core0/DisplayTask.h>
#endif
#ifdef USE_NINTENDOCLASSIC_TASK
#include <tasks/core0/NintendoClassicTask.h>
#endif
#ifdef USE_QWIIC_TASK
#include <tasks/core0/QwiicButtonTaskBase.h>
#endif
#ifdef USE_QWIIC_DISPLAY_TASK
#include <tasks/core0/QwiicDisplayTask.h>
#endif
#ifdef USE_HAPTIC_TASK
#include <tasks/core0/HapticTask.h>
#endif
#ifdef USE_DIGITALPRIMARYBUTTON_TASK
#include <tasks/core0/DigitalPrimaryButtonTask.h>
#endif
#ifdef USE_STATS_TASK
#include <tasks/core0/StatsTask.h>
#endif

#define NUM_TASKS 20