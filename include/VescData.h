#ifndef VescData_h
#define VescData_h

#include "Arduino.h"

enum ReasonType
{
  BOARD_STOPPED,
  BOARD_MOVING,
  FIRST_PACKET,
  LAST_WILL,
  REQUESTED,
  VESC_OFFLINE,
  RESPONSE,
};

enum PacketType
{
  CONTROL,
  CONFIG,
};

enum CommandFlag
{
  MISSED_PACKET,
  UNSUCCESSFUL_REPLY
};

class VescData
{
public:
  float batteryVoltage;
  bool moving;
  float ampHours;
  float motorCurrent;
  float odometer; // in kilometers
  bool vescOnline;
  unsigned long id;
  ReasonType reason;
  // debugging
  uint16_t missedPackets;
  uint16_t unsuccessfulReplies;

  uint32_t flags;
  uint16_t flagValue;
};

class ControllerData
{
public:
  uint8_t throttle;
  unsigned long id;
  uint8_t command;
  bool cruise_control;
  uint32_t ackFlags;
};

void clearFlags(uint16_t flags)
{
  flags = 0;
}

void clearFlag(uint16_t flags, CommandFlag flag)
{
  flags &= ~(1UL << (int)flag);
}

void setFlag(uint16_t flags, CommandFlag flag)
{
  flags |= 1UL << (int)flag;
}

class ControllerConfig
{
public:
  uint16_t send_interval;
  bool cruise_control_enabled;
  unsigned long id;
};

class BoardConfig
{
public:
  unsigned long id;
};

#define COMMAND_REQUEST_UPDATE 1

#endif
