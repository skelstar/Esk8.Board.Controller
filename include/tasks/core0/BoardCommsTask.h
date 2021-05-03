#pragma once

#include <TaskBase.h>
#include <QueueManager.h>
#include <tasks/queues/types/root.h>
#include <tasks/queues/QueueFactory.h>

namespace BoardComms
{
  void sendPacketToBoard(bool print = false);
  void boardPacketAvailable_cb(uint16_t from_id, uint8_t t);
}

class BoardCommsTask : public TaskBase
{
public:
  bool printWarnings = false;
  bool printSentPacketToBoard = false;
  bool printRadioDetails = true;
  bool printRxPacket = false;
  unsigned long sendToBoardInterval = SEND_TO_BOARD_INTERVAL;

  elapsedMillis sinceLastBoardResponse = 0,
                sinceSentToBoard = 0;

  GenericClient<ControllerData, VescData> *boardClient;

  Queue1::Manager<BoardState> *packetStateQueue = nullptr;
  Queue1::Manager<ThrottleState> *throttleStateQueue = nullptr;

  ControllerData controller_packet;
  BoardState packetState;

private:
  // ControllerConfig controller_config;

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
    packetStateQueue = createQueueManager<BoardState>("(BoardCommsTask)PacketStateQueue");
    throttleStateQueue = createQueueManager<ThrottleState>("(BoardCommsTask)ThrottleStateQueue");
  }
  //----------------------------------------------------------
  void initialise()
  {
    if (mux_SPI == nullptr)
      mux_SPI = xSemaphoreCreateMutex();

    packetStateQueue->read(); // clear the queue

    controller_packet.throttle = 127;

    nrf24.begin(&radio, &network, COMMS_CONTROLLER, nullptr, false, printRadioDetails);

    boardClient = new GenericClient<ControllerData, VescData>(COMMS_BOARD);

    boardClient->begin(&network, BoardComms::boardPacketAvailable_cb, mux_SPI);
    Serial.printf("boardClient ready\n");
  }
  //----------------------------------------------------------

  bool timeToDoWork()
  {
    return true;
  }
  //----------------------------------------------------------
  void doWork()
  {
    boardClient->update();

    if (throttleStateQueue->hasValue())
    {
      controller_packet.throttle = throttleStateQueue->payload.val;
    }

    if (sinceSentToBoard > sendToBoardInterval)
    {
      sinceSentToBoard = 0;
      BoardComms::sendPacketToBoard();
    }

    // have to send so display knows when board goes offline, maybe
    // should poll in DisplayTask instead
    // if (sinceLastBoardResponse > sendToBoardInterval)
    // {
    //   packetStateQueue->send(&packetState);
    // }
  }

  void cleanup()
  {
    delete (throttleStateQueue);
  }
  //----------------------------------------------------------
};

BoardCommsTask boardCommsTask;

namespace BoardComms
{
  void task1(void *parameters)
  {
    boardCommsTask.task(parameters);
  }
  //----------------------------------------------------------
  void sendPacketToBoard(bool print)
  {
    boardCommsTask.controller_packet.id++;
    boardCommsTask.controller_packet.acknowledged = false;
    boardCommsTask.controller_packet.txTime = millis(); // board updates txTime

    bool success = boardCommsTask.boardClient->sendTo(Packet::CONTROL, boardCommsTask.controller_packet);

    if (boardCommsTask.printSentPacketToBoard)
      Serial.printf("------------------\nsendPacketToBoard() @ %lums id: %lu \n",
                    boardCommsTask.controller_packet.txTime,
                    boardCommsTask.controller_packet.id);

    boardCommsTask.packetState.sent(boardCommsTask.controller_packet);

    if (!success)
      Serial.printf("Unable to send CONTROL packet to board, id: %lu\n", boardCommsTask.controller_packet.id);
  }
  //----------------------------------------------------------
  void boardPacketAvailable_cb(uint16_t from_id, uint8_t t)
  {
    VescData packet = boardCommsTask.boardClient->read();

    if (boardCommsTask.printRxPacket)
      VescData::print(packet, "[boardPacketAvailable_cb]rx");

    // map packet to BoardState type
    boardCommsTask.packetState.received(packet);

    if (packet.reason == CONFIG_RESPONSE)
    {
      Serial.printf("CONFIG_RESPONSE id: %lu\n", packet.id);
    }

    boardCommsTask.packetStateQueue->send(&boardCommsTask.packetState, boardCommsTask.printSendToQueue ? QueueBase::printSend : nullptr);
    boardCommsTask.sinceLastBoardResponse = 0;

    if (boardCommsTask.printRxPacket)
      VescData::print(packet, "[Packet_cb]");

    vTaskDelay(10);
  }
} // namespace