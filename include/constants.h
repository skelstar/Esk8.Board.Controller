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

#define TICKS_0ms 0
#define TICKS_1ms 1 / portTICK_PERIOD_MS
#define TICKS_2ms 2 / portTICK_PERIOD_MS
#define TICKS_5ms 5 / portTICK_PERIOD_MS
#define TICKS_10ms 10 / portTICK_PERIOD_MS
#define TICKS_50ms 50 / portTICK_PERIOD_MS
#define TICKS_100ms 100 / portTICK_PERIOD_MS
#define TICKS_200ms 200 / portTICK_PERIOD_MS

// ThrottleTask

enum ThrottleStatus
{
  STATUS_OK = 0,
  DIAL_DISCONNECTED,
};

const char *getThrottleStatus(uint8_t status)
{
  switch (status)
  {
  case OK:
    return "OK";
  case DIAL_DISCONNECTED:
    return "DIAL_DISCONNECTED";
  }
  return "OUT OF RANGE (getStatus)";
}

//------------------------------------------------------------

// namespace Comms
// {
//   enum Event
//   {
//     NO_EVENT = 0,
//     PKT_RXD,
//     BOARD_TIMEDOUT,
//     BOARD_FIRST_PACKET,
//     Length
//   };

//   const char *getEventName(uint8_t ev)
//   {
//     switch (ev)
//     {
//     case NO_EVENT:
//       return "NO_EVENT";
//     case PKT_RXD:
//       return "PKT_RXD";
//     case BOARD_TIMEDOUT:
//       return "BOARD_TIMEDOUT";
//     case BOARD_FIRST_PACKET:
//       return "BOARD_FIRST_PACKET";
//     }
//     return "Out of range (Comms::getEventName())";
//   }
// } // namespace Comms

//------------------------------------------------------------

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
#ifndef GIT_COMMIT_HASH
#define GIT_COMMIT_HASH "hash not provided?"
#endif

#ifndef PRINT_BOARD_CLIENT_CONNECTED_CHANGED
#define PRINT_BOARD_CLIENT_CONNECTED_CHANGED 0
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

#ifndef PRINT_NINTENDO_BUTTON
#define PRINT_NINTENDO_BUTTON 0
#endif

#ifndef USING_DISPLAY
#define USING_DISPLAY 0
#endif
#ifndef USING_REMOTE
#define USING_REMOTE 0
#endif
#ifndef USING_STATS
#define USING_STATS 0
#endif
#ifndef USING_NINTENDO_BUTTONS
#define USING_NINTENDO_BUTTONS 0
#endif
#ifndef USING_LED
#define USING_LED 0
#endif
#ifndef USING_DEBUG_TASK
#define USING_DEBUG_TASK 0
#endif
#ifndef USING_QWIIC_BUTTON_TASK
#define USING_QWIIC_BUTTON_TASK 0
#endif

#define BOARD_COMMS_STATE_FORMAT_LONG "[BOARD: %s | %s ]\n"
#define BOARD_COMMS_STATE_FORMAT_SHORT "[BOARD: %s | _ ]\n"
#define BOARD_CLIENT_CONNECTED_FORMAT "BOARD CLIENT: %s\n"

#define TX_TO_BOARD_FORMAT "[TX -> BOARD]: id=%d\n"
#define RX_FROM_BOARD_FORMAT "[RX <- BOARD]: id=%d\n"

enum TriState
{
  STATE_NONE,
  STATE_ON,
  STATE_OFF
};

TriState pulseLedOn;
