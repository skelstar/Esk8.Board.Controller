#define CONSTANTS_H

enum ButtonClickType
{
  NO_CLICK,
  SINGLE,
  DOUBLE,
  TRIPLE,
  LONG_PRESS,
};

#define STORE_STATS "stats"
#define STORE_STATS_SOFT_RSTS "soft resets"
#define STORE_STATS_TRIP_TIME "trip time"

#define OUT_OF_RANGE "OUT OF RANGE"
#define NO_MESSAGE_ON_QUEUE 0

#define TICKS_0 0
#define TICKS_1 1
#define TICKS_2 2
#define TICKS_5 5
#define TICKS_10 10
#define TICKS_50 50
#define TICKS_100 100

//------------------------------------------------------------
namespace DispState
{
  enum Trigger
  {
    NO_EVENT = 0,
    CONNECTED,
    DISCONNECTED,
    STOPPED,
    MOVING,
    UPDATE,
    REMOTE_BATTERY_CHANGED,
    PRIMARY_SINGLE_CLICK,
    PRIMARY_DOUBLE_CLICK,
    PRIMARY_TRIPLE_CLICK,
    PRIMARY_LONG_PRESS,
    VERSION_DOESNT_MATCH,
    RIGHT_BUTTON_CLICKED,
  };

  const char *getTrigger(int ev)
  {
    switch (ev)
    {
    case NO_EVENT:
      return "NO_EVENT";
    case CONNECTED:
      return "CONNECTED";
    case DISCONNECTED:
      return "DISCONNECTED";
    case STOPPED:
      return "STOPPED";
    case MOVING:
      return "MOVING";
    case UPDATE:
      return "UPDATE";
    case REMOTE_BATTERY_CHANGED:
      return "REMOTE_BATTERY_CHANGED";
    case PRIMARY_SINGLE_CLICK:
      return "PRIMARY_SINGLE_CLICK";
    case PRIMARY_DOUBLE_CLICK:
      return "PRIMARY_DOUBLE_CLICK";
    case PRIMARY_TRIPLE_CLICK:
      return "PRIMARY_TRIPLE_CLICK";
    case PRIMARY_LONG_PRESS:
      return "PRIMARY_LONG_PRESS";
    case VERSION_DOESNT_MATCH:
      return "VERSION_DOESNT_MATCH";
    case RIGHT_BUTTON_CLICKED:
      return "RIGHT_BUTTON_CLICKED";
    }
    return OUT_OF_RANGE;
  }
} // namespace DispState

//------------------------------------------------------------

namespace Comms
{
  enum Event
  {
    NO_EVENT = 0,
    PKT_RXD,
    BOARD_TIMEDOUT,
    BOARD_FIRST_PACKET,
    Length
  };

  const char *getEventName(uint8_t ev)
  {
    switch (ev)
    {
    case NO_EVENT:
      return "NO_EVENT";
    case PKT_RXD:
      return "PKT_RXD";
    case BOARD_TIMEDOUT:
      return "BOARD_TIMEDOUT";
    case BOARD_FIRST_PACKET:
      return "BOARD_FIRST_PACKET";
    }
    return "Out of range (Comms::getEventName())";
  }
} // namespace Comms

//------------------------------------------------------------

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
#ifndef PRINT_THROTTLE
#define PRINT_THROTTLE 0
#endif
#ifndef PRINT_RESET_DETECTION
#define PRINT_RESET_DETECTION 0
#endif
#ifndef PRINT_DISP_STATE
#define PRINT_DISP_STATE 0
#endif
#ifndef PRINT_DISP_STATE_EVENT
#define PRINT_DISP_STATE_EVENT 0
#endif
#ifndef PRINT_DISP_QUEUE_READ
#define PRINT_DISP_QUEUE_READ 0
#endif
#ifndef PRINT_IF_TOTAL_FAILED_SENDING
#define PRINT_IF_TOTAL_FAILED_SENDING 0
#endif
#ifndef SUPPRESS_EV_COMMS_PKT_RXD
#define SUPPRESS_EV_COMMS_PKT_RXD 0
#endif
#ifndef PRINT_BUTTON_EVENTS
#define PRINT_BUTTON_EVENTS 0
#endif
#ifndef STORE_SNAPSHOT_INTERVAL
#define STORE_SNAPSHOT_INTERVAL 5000
#endif
#ifndef SUPPRESS_EV_COMMS_PKT_RXD
#define SUPPRESS_EV_COMMS_PKT_RXD 1
#endif
#ifndef PRINT_NRF24L01_DETAILS
#define PRINT_NRF24L01_DETAILS 0
#endif
#ifndef PRINT_TX_TO_BOARD
#define PRINT_TX_TO_BOARD 0
#endif
#ifndef PRINT_RX_FROM_BOARD
#define PRINT_RX_FROM_BOARD 0
#endif
#ifndef DEBUG_BUILD
#define DEBUG_BUILD 0
#endif
#ifndef RELEASE_BUILD
#define RELEASE_BUILD 0
#endif
#ifndef GIT_BRANCH_NAME
#define GIT_BRANCH_NAME "branch not provided?"
#endif

#ifndef PRINT_BOARD_CLIENT_CONNECTED_CHANGED
#define PRINT_BOARD_CLIENT_CONNECTED_CHANGED 0
#endif

#ifndef PRINT_STATS_QUEUE_SEND
#define PRINT_STATS_QUEUE_SEND 0
#endif
#ifndef PRINT_STATS_QUEUE_READ
#define PRINT_STATS_QUEUE_READ 0
#endif

#ifndef PRINT_COMMS_QUEUE_SENT
#define PRINT_COMMS_QUEUE_SENT 0
#endif

#ifndef PRINT_COMMS_QUEUE_READ
#define PRINT_COMMS_QUEUE_READ 0
#endif

#ifndef FEATURE_LED_COUNT
#define FEATURE_LED_COUNT 0
#endif

#ifndef FEATURE_PUSH_TO_START
#define FEATURE_PUSH_TO_START 0
#endif
#ifndef FEATURE_START_MOVING_BOOST
#define FEATURE_START_MOVING_BOOST 0
#endif

#ifndef PRINT_STATS_MUTEX_TAKE_STATE
#define PRINT_STATS_MUTEX_TAKE_STATE 0
#endif

#ifndef PRINT_STATS_MUTEX_GIVE_STATE
#define PRINT_STATS_MUTEX_GIVE_STATE 0
#endif

#ifndef OPTION_USING_MAG_THROTTLE
#define OPTION_USING_MAG_THROTTLE 0
#endif

#define BOARD_COMMS_STATE_FORMAT_LONG "[BOARD: %s | %s ]\n"
#define BOARD_COMMS_STATE_FORMAT_SHORT "[BOARD: %s | _ ]\n"
#define BOARD_CLIENT_CONNECTED_FORMAT "BOARD CLIENT: %s\n"

#define TX_TO_BOARD_FORMAT "[TX -> BOARD]: id=%d\n"
#define RX_FROM_BOARD_FORMAT "[RX <- BOARD]: id=%d\n"
#define PRINT_TASK_STARTED_FORMAT "TASK: %s on Core %d\n"

enum TriState
{
  STATE_NONE,
  STATE_ON,
  STATE_OFF
};

TriState pulseLedOn;
