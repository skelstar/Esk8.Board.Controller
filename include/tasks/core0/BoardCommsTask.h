#pragma once

#include <TaskBaseAlt.h>
#include <QueueManager.h>
#include <tasks/queues/types/root.h>
#include <tasks/queues/QueueFactory.h>

NRF24L01Lib nrf24;
RF24 radio(NRF_CE, NRF_CS, RF24_SPI_SPEED);
RF24Network network(radio);

namespace BoardComms
{
  void boardPacketAvailable_cb(uint16_t from_id, uint8_t t);
}

class BoardCommsTask : public TaskBaseAlt
{
public:
  bool printWarnings = false;
  bool printSentPacketToBoard = false;
  bool printRadioDetails = true;
  unsigned long SEND_TO_BOARD_INTERVAL_LOCAL = SEND_TO_BOARD_INTERVAL;

  elapsedMillis since_last_response = 0;

  BoardClass board;

  PacketState packetState;

#ifndef MOCK_GENERIC_CLIENT
#include <GenericClient.h>
#endif
  GenericClient<ControllerData, VescData> *boardClient; //(COMMS_BOARD);

  Queue1::Manager<PacketState> *packetStateQueue = nullptr;

private:
  // ControllerConfig controller_config;
  ControllerData controller_packet;

  elapsedMillis
      since_checked_for_available,
      since_sent_to_board = 0;

public:
  BoardCommsTask(unsigned long p_doWorkInterval) : TaskBaseAlt("BoardCommsTask", 3000, p_doWorkInterval)
  {
  }
  //----------------------------------------------------------
  void sendPacketToBoard(bool print = false)
  {
    controller_packet.id++;
    controller_packet.acknowledged = false;

    if (printSentPacketToBoard)
      Serial.printf("sendPacketToBoard() @%lums id: %lu enabled: %d\n", millis(), controller_packet.id, enabled);

    bool success = boardClient->sendTo(Packet::CONTROL, controller_packet);

    packetState.sent(controller_packet);

    if (!success)
      Serial.printf("Unable to send CONTROL packet to board, id: %lu\n", controller_packet.id);
  }
  //----------------------------------------------------------
  void initialiseQueues()
  {
    packetStateQueue = createQueue<PacketState>("(BoardCommsTask)PacketStateQueue");
  }
  //----------------------------------------------------------
  void initialise()
  {
    if (mux_SPI == nullptr)
      mux_SPI = xSemaphoreCreateMutex();

    packetStateQueue->read(); // clear the queue

    nrf24.begin(&radio, &network, COMMS_CONTROLLER, nullptr, false, printRadioDetails);

    boardClient = new GenericClient<ControllerData, VescData>(COMMS_BOARD);

    boardClient->begin(&network, BoardComms::boardPacketAvailable_cb, mux_SPI);
    // boardClient->printWarnings = printWarnings;
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

      sendPacketToBoard();
    }

    if (since_last_response > SEND_TO_BOARD_INTERVAL_LOCAL)
    {
      packetStateQueue->send(&packetState);
    }
  }
  //----------------------------------------------------------
  void start(uint8_t priority, TaskFunction_t taskRef)
  {
    rtos->create(taskRef, CORE_1, priority, WITH_HEALTHCHECK);
  }
  //----------------------------------------------------------
  void deleteTask(bool print = false)
  {
    if (rtos != nullptr)
      rtos->deleteTask(print);
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

  void boardPacketAvailable_cb(uint16_t from_id, uint8_t t)
  {
    VescData packet = boardCommsTask.boardClient->read();
    boardCommsTask.packetState.received(packet);
    boardCommsTask.board.save(packet);

    if (packet.reason == CONFIG_RESPONSE)
    {
      Serial.printf("CONFIG_RESPONSE id: %lu\n", packet.id);
    }
    // Serial.printf("[BoardComms|%lu] packet.id: %lu\n", packet.id);

    boardCommsTask.packetStateQueue->send(&boardCommsTask.packetState, boardCommsTask.printSendToQueue ? QueueBase::printSend : nullptr);
    boardCommsTask.since_last_response = 0;

    vTaskDelay(10);
  }

}