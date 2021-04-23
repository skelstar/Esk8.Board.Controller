#pragma once

#include <TaskBase.h>
#include <QueueManager.h>
#include <tasks/queues/types/root.h>
#include <tasks/queues/QueueFactory.h>

namespace BoardCommsTask
{
  bool printWarnings = false;

  TaskBase *thisTask;

  NRF24L01Lib nrf24;
  RF24 radio(NRF_CE, NRF_CS, RF24_SPI_SPEED);
  RF24Network network(radio);

#ifndef MOCK_GENERIC_CLIENT
#include <GenericClient.h>
#endif

  GenericClient<ControllerData, VescData> boardClient(COMMS_BOARD);

  namespace
  {
    Queue1::Manager<PacketState> *packetStateQueue = nullptr;

    // ControllerConfig controller_config;
    ControllerData controller_packet;

    BoardClass board;

    PacketState packetState;

    bool printSentPacketToBoard = false;
    unsigned long SEND_TO_BOARD_INTERVAL_LOCAL = SEND_TO_BOARD_INTERVAL;
    elapsedMillis
        since_checked_for_available,
        since_last_response,
        since_sent_to_board = 0,
        since_last_did_work = 0;

    //----------------------------------------------------------
    void boardPacketAvailable_cb(uint16_t from_id, uint8_t t)
    {
      VescData packet = boardClient.read();
      packetState.received(packet);
      board.save(packet);

      if (packet.reason == CONFIG_RESPONSE)
      {
        Serial.printf("CONFIG_RESPONSE id: %lu\n", packet.id);
      }
      // Serial.printf("[BoardComms|%lu] packet.id: %lu\n", packet.id);

      packetStateQueue->send(&packetState, thisTask->printSendToQueue ? QueueBase::printSend : nullptr);
      since_last_response = 0;

      vTaskDelay(10);
    }
    //----------------------------------------------------------
    void sendPacketToBoard(bool print = false)
    {
      controller_packet.id++;
      controller_packet.acknowledged = false;

      if (printSentPacketToBoard)
        Serial.printf("sendPacketToBoard() @%lums id: %lu enabled: %d\n", millis(), controller_packet.id, thisTask->enabled);

      bool success = boardClient.sendTo(Packet::CONTROL, controller_packet);

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
      packetStateQueue->read(); // clear the queue

      nrf24.begin(&radio, &network, COMMS_CONTROLLER);

      boardClient.begin(&network, boardPacketAvailable_cb, mux_SPI);
      // boardClient.printWarnings = printWarnings;
    }
    //----------------------------------------------------------

    bool timeToDowork()
    {
      return true;
    }
    //----------------------------------------------------------
    void doWork()
    {
      boardClient.update();

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
    void task(void *parameters)
    {
      thisTask->task(parameters);
    }
  }

  void start(uint8_t priority, ulong doWorkInterval, ulong sendToBoardInterval)
  {
    thisTask = new TaskBase("BoardCommsTask", 3000);
    thisTask->setInitialiseCallback(initialise);
    thisTask->setInitialiseQueuesCallback(initialiseQueues);
    thisTask->setTimeToDoWorkCallback(timeToDowork);
    thisTask->setDoWorkCallback(doWork);

    thisTask->doWorkInterval = doWorkInterval;
    SEND_TO_BOARD_INTERVAL_LOCAL = sendToBoardInterval;

    if (thisTask->rtos != nullptr)
      thisTask->rtos->create(task, CORE_1, priority, WITH_HEALTHCHECK);
  }

  void deleteTask(bool print = false)
  {
    if (thisTask != nullptr && thisTask->rtos != nullptr)
      thisTask->rtos->deleteTask(print);
  }
}