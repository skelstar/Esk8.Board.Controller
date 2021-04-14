#include <Arduino.h>

enum QueueType
{
  QT_NONE = 0,
  QT_PacketState,
  QT_Notification,
  QT_BoardClass,
  QT_NintendoButtonEvent,
  QT_PrimaryButtonState,
  QT_ThrottleState,
  QT_SendToBoardNotf,
  QT_OTHER,
};

static const char *getQueueType(uint8_t p_type)
{
  switch (p_type)
  {
  case QT_NONE:
    return "QT_NONE";
  case QT_PacketState:
    return "QT_PacketState";
  case QT_Notification:
    return "QT_Notification";
  case QT_BoardClass:
    return "QT_BoardClass";
  case QT_NintendoButtonEvent:
    return "QT_NintendoButtonEvent";
  case QT_PrimaryButtonState:
    return "QT_PrimaryButtonState";
  case QT_ThrottleState:
    return "QT_ThrottleState";
  case QT_SendToBoardNotf:
    return "QT_SendToBoardNotf";
  case QT_OTHER:
    return "QT_OTHER";
  }
  return "OUT OF RANGE: getQueueType";
}