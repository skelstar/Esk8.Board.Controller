#pragma once

#include <VescData.h>
#include <QueueBase.h>
#include <elapsedMillis.h>

class Transaction : public QueueBase
{
public:
  enum TransactionSendResult
  {
    NONE = 0,
    OK,
    FAIL,
    UPDATE,
  };

  const char *getSendResult()
  {
    switch (this->sendResult)
    {
    case NONE:
      return "NONE";
    case OK:
      return "OK";
    case FAIL:
      return "FAIL";
    case UPDATE:
      return "UPDATE";
    }
    // TODO build method that does this __func__
    return "OUT_OF_RANGE (getSendResult)";
  }

  TransactionSendResult sendResult;
  float version = 0.0,
        batteryVolts = 0.0;
  unsigned long
      packet_id,
      replyId,
      responseTime;
  bool moving = false;

public:
  Transaction() : QueueBase()
  {
    event_id = 0;
    packet_id = 0;
    name = "Transaction";
  }

  void start(ControllerData packet)
  {
    packet_id = packet.id;
  }

  void start(ControllerConfig config_packet)
  {
    packet_id = config_packet.id;
  }

  void received(VescData packet)
  {
    replyId = packet.id;
    responseTime = millis();
    version = packet.version;
    moving = packet.moving;
    batteryVolts = packet.batteryVoltage;
    sendResult = TransactionSendResult::UPDATE;
  }

  bool acknowledged()
  {
    return packet_id == replyId;
  }

  bool connected(unsigned long timeout)
  {
    return sendResult != TransactionSendResult::FAIL;
  }

  void print(const char *preamble = nullptr)
  {
    if (preamble != nullptr)
      Serial.printf("%s: ", preamble);
    Serial.printf("event_id: %lu  ", this->event_id);
    Serial.printf("packet_id: %lu:  ", this->packet_id);
    Serial.printf("moving: %d:  ", this->moving);
    Serial.printf("sentResult: %s:  ", this->getSendResult());
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
};