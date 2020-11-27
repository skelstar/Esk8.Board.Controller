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
  CONTROL = 0,
  CONFIG,
  HUD,
};

enum CommandType
{
  NONE,
  RESET,
};

class VescData
{
public:
  unsigned long id;
  float batteryVoltage;
  bool moving;
  float ampHours;     // total number of ampHours used
  float motorCurrent; // current motor current
  float odometer;     // in kilometers
  bool vescOnline;
  ReasonType reason;
  CommandType command;
  float version;
};

class ControllerData
{
public:
  unsigned long id;
  uint8_t throttle;
  bool cruise_control;
  uint8_t command;
};

class ControllerConfig
{
public:
  unsigned long id;
  uint16_t send_interval;
};

#endif
