#pragma once

#include <TaskBase.h>
#include <QueueManager.h>
#include <tasks/queues/types/root.h>
#include <tasks/queues/QueueFactory.h>

namespace nsBoardComms
{
  bool sendPacketToBoard(bool print = false);
  void boardPacketAvailable_cb(uint16_t from_id, uint8_t t);
}

class BoardCommsTask : public TaskBase
{
public:
  bool printWarnings = false;
  bool printRadioDetails = true;
  bool printSentPacketToBoard = false,
       printBoardPacketAvailable = false;
  bool printRxQueuePacket = false,
       printTxQueuePacket = false;

  elapsedMillis sinceSentToBoard = 0;

  GenericClient<ControllerData, VescData> *boardClient;

  Queue1::Manager<Transaction> *boardTransactionQueue = nullptr;
  Queue1::Manager<ThrottleState> *throttleStateQueue = nullptr;

  ControllerData controller_packet;
  Transaction transaction;

private:
  elapsedMillis since_checked_for_available;

public:
  BoardCommsTask() : TaskBase("BoardCommsTask", 3000, PERIOD_50ms)
  {
    _core = CORE_1;
    _priority = TASK_PRIORITY_4;
  }
  //----------------------------------------------------------
  void initialiseQueues()
  {
    boardTransactionQueue = createQueueManager<Transaction>("(BoardCommsTask)BoardTransactionQueue");
    throttleStateQueue = createQueueManager<ThrottleState>("(BoardCommsTask)ThrottleStateQueue");

    boardTransactionQueue->read(); // clear the queue
  }
  //----------------------------------------------------------
  void initialise()
  {
    if (mux_SPI == nullptr)
      mux_SPI = xSemaphoreCreateMutex();

    controller_packet.throttle = 127;

    nrf24.begin(&radio, &network, COMMS_CONTROLLER, nullptr, false, printRadioDetails);

    boardClient = new GenericClient<ControllerData, VescData>(COMMS_BOARD);

    boardClient->begin(&network, nsBoardComms::boardPacketAvailable_cb, mux_SPI);
    Serial.printf("boardClient ready\n");
  }
  //----------------------------------------------------------

  void doWork()
  {
    boardClient->update();

    if (throttleStateQueue->hasValue())
    {
      controller_packet.throttle = throttleStateQueue->payload.val;
    }

    // time to send to board
    if (sinceSentToBoard > SEND_TO_BOARD_INTERVAL)
    {
      sinceSentToBoard = 0;

      bool sentOK = nsBoardComms::sendPacketToBoard();

      transaction.sendResult = sentOK ? Transaction::SENT_OK : Transaction::SEND_FAIL;
      transaction.start(controller_packet);

      boardTransactionQueue->send(&transaction, printSendToQueue ? QueueBase::printSend : nullptr);

      if (printSentPacketToBoard)
        _printSentPacketToBoard(controller_packet, transaction.sendResult == Transaction::SENT_OK);
    }
  }

  void cleanup()
  {
    delete (throttleStateQueue);
  }
  //----------------------------------------------------------

private:
  void _printSentPacketToBoard(const ControllerData &packet, bool success)
  {
    if (success)
    {
      Serial.printf("------------------\nsendPacketToBoard() @ %lums id: %lu sentOK \n",
                    packet.txTime, packet.id);
    }
    else
    {
      Serial.printf("x");
    }
  }
};

//============================================================

BoardCommsTask boardCommsTask;

namespace nsBoardComms
{
  //----------------------------------------------------------
  void task1(void *parameters)
  {
    boardCommsTask.task(parameters);
  }
  //----------------------------------------------------------
  bool sendPacketToBoard(bool print)
  {
    boardCommsTask.controller_packet.id++;
    boardCommsTask.controller_packet.acknowledged = false;
    boardCommsTask.controller_packet.txTime = millis(); // board updates txTime

    bool success = boardCommsTask.boardClient->sendTo(Packet::CONTROL, boardCommsTask.controller_packet);

    return success;
  }
  //----------------------------------------------------------
  void boardPacketAvailable_cb(uint16_t from_id, uint8_t t)
  {
    VescData packet = boardCommsTask.boardClient->read();

    if (boardCommsTask.printBoardPacketAvailable)
      VescData::print(packet, "-->[boardPacketAvailable_cb|NRF24]");

    // map packet to Transaction type
    boardCommsTask.transaction.received(packet);

    if (packet.reason == CONFIG_RESPONSE)
    {
      Serial.printf("CONFIG_RESPONSE id: %lu\n", packet.id);
    }

    boardCommsTask.boardTransactionQueue->send(&boardCommsTask.transaction, boardCommsTask.printSendToQueue ? QueueBase::printSend : nullptr);

    if (boardCommsTask.printTxQueuePacket)
      packet.printThis("[Packet_cb|transactionQueue]->");

    vTaskDelay(10);
  }
  //----------------------------------------------------------
} // namespace