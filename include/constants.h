

enum ButtonClickType
{
  SINGLE,
  DOUBLE,
  TRIPLE
};

#define CORE_0 0
#define CORE_1 1

#define TASK_PRIORITY_1 1
#define TASK_PRIORITY_2 2
#define TASK_PRIORITY_3 3
#define TASK_PRIORITY_4 4
#define TASK_PRIORITY_5 5

#define STORE_STATS "stats"
#define STORE_STATS_SOFT_RSTS "soft resets"
#define STORE_STATS_TRIP_TIME "trip time"

enum DispStateEvent
{
  DISP_EV_NO_EVENT = 0,
  DISP_EV_CONNECTED,
  DISP_EV_DISCONNECTED,
  DISP_EV_STOPPED,
  DISP_EV_MOVING,
  DISP_EV_SW_RESET,
  DISP_EV_UPDATE,
  DISP_EV_PRIMARY_SINGLE_CLICK,
  DISP_EV_PRIMARY_DOUBLE_CLICK,
  DISP_EV_PRIMARY_TRIPLE_CLICK,
  DISP_EV_VERSION_DOESNT_MATCH
};

const char *dispStateEventNames[] = {
    "DISP_EV_NO_EVENT",
    "DISP_EV_CONNECTED",
    "DISP_EV_DISCONNECTED",
    "DISP_EV_STOPPED",
    "DISP_EV_MOVING",
    "DISP_EV_SW_RESET",
    "DISP_EV_UPDATE",
    "DISP_EV_PRIMARY_SINGLE_CLICK",
    "DISP_EV_PRIMARY_DOUBLE_CLICK",
    "DISP_EV_PRIMARY_TRIPLE_CLICK",
    "DISP_EV_VERSION_DOESNT_MATCH",
};

enum HudActionEvent
{
  EV_HUD_NONE = 0,
  EV_HUD_DOUBLE_CLICK,
};

const char *hudActionEventNames[] = {
    "EV_HUD_NONE",
    "EV_HUD_DOUBLE_CLICK",
};

enum HUDEvent
{
  HUD_EV_CONNECTED = 0,
  HUD_EV_PULSE_RED,
  HUD_EV_FLASH_GREEN,
  HUD_EV_SPIN_GREEN,
};

const char *eventNames[] = {
    "HUD_EV_CONNECTED",
    "HUD_EV_PULSE_RED",
    "HUD_EV_FLASH_GREEN",
    "HUD_EV_SPIN_GREEN",
};

enum CommsStateEvent
{
  EV_COMMS_NO_EVENT = 0,
  EV_COMMS_PKT_RXD,
  EV_COMMS_BOARD_TIMEDOUT,
  EV_COMMS_BD_FIRST_PACKET,
};

const char *commsStateEventNames[] = {
    "EV_COMMS_NO_EVENT",
    "EV_COMMS_PKT_RXD",
    "EV_COMMS_BOARD_TIMEDOUT",
    "EV_COMMS_BD_FIRST_PACKET",
};

#define LCD_WIDTH 240
#define LCD_HEIGHT 135

#define TFT_DEFAULT_BG TFT_BLACK

//-----------------------------------------------------
// build flag defaults

#ifndef PRINT_COMMS_STATE
#define PRINT_COMMS_STATE 0
#endif
#ifndef PRINT_COMMS_STATE_EVENT
#define PRINT_COMMS_STATE_EVENT 0
#endif
#ifndef SUPPRESS_EV_COMMS_PKT_RXD
#define SUPPRESS_EV_COMMS_PKT_RXD 0
#endif
#ifndef FEATURE_CRUISE_CONTROL
#define FEATURE_CRUISE_CONTROL false
#endif
#ifndef FEATURE_PUSH_TO_START
#define FEATURE_PUSH_TO_START false
#endif
#ifndef PRINT_BUTTON_EVENTS
#define PRINT_BUTTON_EVENTS 0
#endif
//-----------------------------------------------------

enum TriState
{
  STATE_NONE,
  STATE_ON,
  STATE_OFF
};

TriState pulseLedOn;