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
  unsigned long SEND_TO_BOARD_INTERVAL_LOCAL = SEND_TO_BOARD_INTERVAL;

  elapsedMillis since_last_response = 0,
                since_sent_to_board = 0;

  BoardClass board;

  PacketState packetState;

  GenericClient<ControllerData, VescData> *boardClient;

  Queue1::Manager<PacketState> *packetStateQueue = nullptr;
  Queue1::Manager<ThrottleState> *throttleStateQueue = nullptr;

  ControllerData controller_packet;

private:
  // ControllerConfig controller_config;

  elapsedMillis since_checked_for_available;

public:
  BoardCommsTask(unsigned long p_doWorkInterval) : TaskBase("BoardCommsTask", 3000, p_doWorkInterval)
  {
    _core = CORE_1;
    _priority = TASK_PRIORITY_4;
  }
  //----------------------------------------------------------
  void initialiseQueues()
  {
    packetStateQueue = createQueueManager<PacketState>("(BoardCommsTask)PacketStateQueue");
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

    if (since_sent_to_board > SEND_TO_BOARD_INTERVAL_LOCAL)
    {
      since_sent_to_board = 0;

      BoardComms::sendPacketToBoard();
    }

    if (throttleStateQueue->hasValue())
    {
      controller_packet.throttle = throttleStateQueue->payload.val;
    }

    if (since_last_response > SEND_TO_BOARD_INTERVAL_LOCAL)
    {
      packetStateQueue->send(&packetState);
    }
  }

  void cleanup()
  {
    delete (throttleStateQueue);
  }
  //----------------------------------------------------------
};

BoardCommsTask boardCommsTask(PERIOD_100ms);

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
    boardCommsTask.controller_packet.txTime = millis();

    bool success = boardCommsTask.boardClient->sendTo(Packet::CONTROL, boardCommsTask.controller_packet);

    if (boardCommsTask.printSentPacketToBoard)
      Serial.printf("sendPacketToBoard() @ %lums id: %lu \n",
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
    VescData::print(packet, "[boardPacketAvailable_cb]rx");
    // map packet to PacketState type
    boardCommsTask.packetState.received(packet);
    boardCommsTask.board.save(packet);

    if (packet.reason == CONFIG_RESPONSE)
    {
      Serial.printf("CONFIG_RESPONSE id: %lu\n", packet.id);
    }

    boardCommsTask.packetStateQueue->send(&boardCommsTask.packetState, boardCommsTask.printSendToQueue ? QueueBase::printSend : nullptr);
    boardCommsTask.since_last_response = 0;

    if (boardCommsTask.printRxPacket)
      VescData::print(packet, "[Packet_cb]");

    vTaskDelay(10);
  }
} // namespace