#pragma once

#include <VescData.h>
#include <QueueBase.h>
#include <elapsedMillis.h>
#include <shared-utils.h>

class Transaction : public QueueBase
{
public:
  enum SourceType
  {
    CONTROLLER,
    BOARD_RESPONSE,
  };

  static const char *getSourceType(SourceType type)
  {
    switch (type)
    {
    case CONTROLLER:
      return "CONTROLLER";
    case BOARD_RESPONSE:
      return "BOARD_RESPONSE";
    }
    return getOutOfRange("getSourceType");
  }

  float version = 0.0,
        batteryVolts = 0.0;
  ReasonType reason;
  unsigned long
      packet_id,
      replyId;
  bool moving = false;
  SourceType source;
  unsigned long sentTime = 0, roundTrip = 0;

public:
  Transaction() : QueueBase()
  {
    event_id = 0;
    packet_id = 0;
    name = "Transaction";
  }

  void registerPacket(ControllerData packet)
  {
    packet_id = packet.id;
    source = SourceType::CONTROLLER;
    sentTime = packet.txTime;
  }

  void registerPacket(ControllerConfig config_packet)
  {
    packet_id = config_packet.id;
  }

  void reconcile(VescData packet)
  {
    if (packet.id == packet_id)
    {
      // this is the response to current packet
      replyId = packet.id;
      version = packet.version;
      source = SourceType::BOARD_RESPONSE;
      roundTrip = millis() - packet.txTime;
      // sendResult = TransactionSendResult::UPDATE;
    }
    batteryVolts = packet.batteryVoltage;
    moving = packet.moving;
    reason = packet.reason;
    _rxTime = millis();
  }

  bool connected(unsigned long timeout)
  {
    return millis() - _rxTime < timeout;
  }

  void print(const char *preamble = nullptr)
  {
    if (preamble != nullptr)
      Serial.printf("%s: ", preamble);
    Serial.printf("event_id: %lu  ", this->event_id);
    Serial.printf("packet_id: %lu:  ", this->packet_id);
    Serial.printf("battery: %.1fv:  ", this->batteryVolts);
    Serial.printf("moving: %d:  ", this->moving);
    Serial.println();
  }

  static void print(Transaction item, const char *preamble = nullptr)
  {
    if (preamble != nullptr)
      Serial.printf("%s: ", preamble);
    Serial.printf("event_id: %lu  ", item.event_id);
    Serial.printf("packet_id: %lu:  ", item.packet_id);
    Serial.printf("moving: %d:  ", item.moving);
    Serial.println();
  }

private:
  elapsedMillis _since_responded;
  unsigned long _rxTime = 0;
};