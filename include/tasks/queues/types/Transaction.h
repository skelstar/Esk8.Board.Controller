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
    SENT_OK,
    SEND_FAIL,
    UPDATE,
  };

  const char *getSendResult(const char *context = __func__)
  {
    switch (this->sendResult)
    {
    case NONE:
      return "NONE";
    case SENT_OK:
      return "SENT_OK";
    case SEND_FAIL:
      return "SEND_FAIL";
    case UPDATE:
      return "UPDATE";
    }
    char buff[30];
    sprintf(buff, "OUT_OF_RANGE (%s)", context);
    return buff;
  }

  TransactionSendResult sendResult;

  float version = 0.0,
        batteryVolts = 0.0;
  ReasonType reason;
  unsigned long
      packet_id,
      replyId;
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
    batteryVolts = packet.batteryVoltage;
    moving = packet.moving;
    reason = packet.reason;
    replyId = packet.id;
    version = packet.version;

    sendResult = TransactionSendResult::UPDATE;
  }

  bool acknowledged()
  {
    return packet_id == replyId;
  }

  bool connected(unsigned long timeout)
  {
    return sendResult != TransactionSendResult::SEND_FAIL;
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