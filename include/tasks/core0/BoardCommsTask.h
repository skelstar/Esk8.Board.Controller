#pragma once

#include <TaskBase.h>
#include <QueueManager.h>
#include <tasks/queues/types/root.h>
#include <tasks/queues/QueueFactory.h>

#define BOARDCOMMS_TASK

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
       printBoardPacketAvailable = false,
       printFirstBoardPacketAvailable = false;
  bool printRxQueuePacket = false,
       printTxQueuePacket = false;

  elapsedMillis sinceSentToBoard = 0;

  GenericClient<ControllerData, VescData> *boardClient;

  Queue1::Manager<Transaction> *boardTransactionQueue = nullptr;
  Queue1::Manager<ThrottleState> *throttleStateQueue = nullptr;

  ControllerData controller_packet;

private:
  elapsedMillis since_checked_for_available;

public:
  BoardCommsTask() : TaskBase("BoardCommsTask", 3000)
  {
    _core = CORE_1;
  }

  //----------------------------------------------------------
  void initialiseQueues()
  {
    boardTransactionQueue = createQueueManager<Transaction>("(BoardCommsTask)BoardTransactionQueue");
    throttleStateQueue = createQueueManager<ThrottleState>("(BoardCommsTask)ThrottleStateQueue");
    throttleStateQueue->printMissedPacket = false;

    boardTransactionQueue->read(); // clear the queue
  }
  //----------------------------------------------------------
  void _initialise()
  {
    if (mux_SPI == nullptr)
      mux_SPI = xSemaphoreCreateMutex();

    controller_packet.throttle = 127;
    controller_packet.sendInterval = SEND_TO_BOARD_INTERVAL;

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

      boardTransactionQueue->payload.registerPacket(controller_packet);

      boardTransactionQueue->sendPayload();
    }
  }

  void cleanup()
  {
    delete (throttleStateQueue);
  }
  //----------------------------------------------------------

private:
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
    boardCommsTask.controller_packet.sendInterval = SEND_TO_BOARD_INTERVAL;

    bool success = boardCommsTask.boardClient->sendTo(Packet::CONTROL, boardCommsTask.controller_packet);

    return success;
  }
  //-----------------------------------------------------------
  void boardPacketAvailable_cb(uint16_t from_id, uint8_t t)
  {
    VescData packet = boardCommsTask.boardClient->read();

    if (boardCommsTask.printBoardPacketAvailable ||
        (packet.reason == ReasonType::FIRST_PACKET && boardCommsTask.printFirstBoardPacketAvailable))
      packet.print(boardCommsTask._name, __func__); //, "-->[boardPacketAvailable_cb|NRF24]");

    // map packet to Transaction type
    boardCommsTask.boardTransactionQueue->payload.reconcile(packet);

    boardCommsTask.boardTransactionQueue->sendPayload();

    vTaskDelay(10);
  }
  //----------------------------------------------------------
} // namespace