

enum ButtonClickType
{
  SINGLE,
  DOUBLE,
  TRIPLE
};

#define STORE_STATS "stats"
#define STORE_STATS_SOFT_RSTS "soft resets"
#define STORE_STATS_TRIP_TIME "trip time"

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

  const char *names[] = {
      "NO_EVENT",
      "CONNECTED",
      "DISCONNECTED",
      "STOPPED",
      "MOVING",
      "SW_RESET",
      "UPDATE",
      "PRIMARY_SINGLE_CLICK",
      "PRIMARY_DOUBLE_CLICK",
      "PRIMARY_TRIPLE_CLICK",
      "VERSION_DOESNT_MATCH",
  };

  void assertThis()
  {
    assertEnum("DispState", Length, ARRAY_SIZE(names));
  }
} // namespace DispState
//------------------------------------------------------------
namespace CommsState
{
  enum Event
  {
    NO_EVENT = 0,
    PKT_RXD,
    BOARD_TIMEDOUT,
    BD_FIRST_PACKET,
    Length, // leave this one (used for aserting)
  };

  const char *names[] = {
      "NO_EVENT",
      "PKT_RXD",
      "BOARD_TIMEDOUT",
      "BD_FIRST_PACKET",
  };

  void assertThis()
  {
    assertEnum("CommsState", Length, ARRAY_SIZE(names));
  }
} // namespace CommsState
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
