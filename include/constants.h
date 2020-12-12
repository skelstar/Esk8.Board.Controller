#define CONSTANTS_H

enum ButtonClickType
{
  NO_CLICK,
  SINGLE,
  DOUBLE,
  TRIPLE
};

#define STORE_STATS "stats"
#define STORE_STATS_SOFT_RSTS "soft resets"
#define STORE_STATS_TRIP_TIME "trip time"

#define OUT_OF_RANGE "OUT OF RANGE"

//------------------------------------------------------------
namespace DispState
{
  enum Event
  {
    NO_EVENT = 0,
    CONNECTED,
    DISCONNECTED,
    STOPPED,
    MOVING,
    SW_RESET,
    UPDATE,
    PRIMARY_SINGLE_CLICK,
    PRIMARY_DOUBLE_CLICK,
    PRIMARY_TRIPLE_CLICK,
    VERSION_DOESNT_MATCH,
    Length, // leave this one (used for aserting)
  };

  const char *getEvent(int ev)
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
    case SW_RESET:
      return "SW_RESET";
    case UPDATE:
      return "UPDATE";
    case PRIMARY_SINGLE_CLICK:
      return "PRIMARY_SINGLE_CLICK";
    case PRIMARY_DOUBLE_CLICK:
      return "PRIMARY_DOUBLE_CLICK";
    case PRIMARY_TRIPLE_CLICK:
      return "PRIMARY_TRIPLE_CLICK";
    case VERSION_DOESNT_MATCH:
      return "VERSION_DOESNT_MATCH";
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
    Length, // leave this one (used for aserting)
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
    return OUT_OF_RANGE;
  }
} // namespace Comms

//------------------------------------------------------------

namespace HUDTask
{
  enum Message
  {
    NONE,
    BOARD_DISCONNECTED,
    BOARD_CONNECTED,
    WARNING_ACK,
    CONTROLLER_RESET,
    BOARD_MOVING,
    BOARD_STOPPED,
    HEARTBEAT,
    ACKNOWLEDGE,
    CYCLE_BRIGHTNESS,
    THREE_FLASHES,
    GO_TO_IDLE,
    MessageLength,
  };

  const char *getName(uint8_t message)
  {
    switch (message)
    {
    case NONE:
      return "NONE";
    case BOARD_DISCONNECTED:
      return "BOARD_DISCONNECTED";
    case BOARD_CONNECTED:
      return "BOARD_CONNECTED";
    case WARNING_ACK:
      return "WARNING_ACK";
    case CONTROLLER_RESET:
      return "CONTROLLER_RESET";
    case BOARD_MOVING:
      return "BOARD_MOVING";
    case BOARD_STOPPED:
      return "BOARD_STOPPED";
    case HEARTBEAT:
      return "HEARTBEAT";
    case ACKNOWLEDGE:
      return "ACKNOWLEDGE";
    case CYCLE_BRIGHTNESS:
      return "CYCLE_BRIGHTNESS";
    case THREE_FLASHES:
      return "THREE_FLASHES";
    case GO_TO_IDLE:
      return "GO_TO_IDLE";
    }
    return OUT_OF_RANGE;
  }
}; // namespace HUDTask

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
#ifndef PRINT_IF_TOTAL_FAILED_SENDING
#define PRINT_IF_TOTAL_FAILED_SENDING 0
#endif
#ifndef SUPPRESS_EV_COMMS_PKT_RXD
#define SUPPRESS_EV_COMMS_PKT_RXD 0
#endif
#ifndef PRINT_BUTTON_EVENTS
#define PRINT_BUTTON_EVENTS 0
#endif
#ifndef PRINT_HUD_COMMS
#define PRINT_HUD_COMMS 0
#endif
#ifndef PRINT_HUD_MESSAGE_QUEUE_COMMS
#define PRINT_HUD_MESSAGE_QUEUE_COMMS 0
#endif
#ifndef STORE_SNAPSHOT_INTERVAL
#define STORE_SNAPSHOT_INTERVAL 5000
#endif
#ifndef SUPPRESS_EV_COMMS_PKT_RXD
#define SUPPRESS_EV_COMMS_PKT_RXD 1
#endif
#ifndef IGNORE_IF_HUD_OFFLINE
#define IGNORE_IF_HUD_OFFLINE 0
#endif
#ifndef STATE_STRING_FORMAT
#define STATE_STRING_FORMAT "[%s | %s]\n"
#endif

//-----------------------------------------------------

enum TriState
{
  STATE_NONE,
  STATE_ON,
  STATE_OFF
};

TriState pulseLedOn;
