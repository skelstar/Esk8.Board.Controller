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
