#define CONSTANTS_H

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

#ifndef PRINT_DISP_STATE
#define PRINT_DISP_STATE 0
#endif
#ifndef PRINT_DISP_STATE_EVENT
#define PRINT_DISP_STATE_EVENT 0
#endif
#ifndef PRINT_THROTTLE
#define PRINT_THROTTLE 0
#endif
#ifndef PRINT_NRF24L01_DETAILS
#define PRINT_NRF24L01_DETAILS 0
#endif
