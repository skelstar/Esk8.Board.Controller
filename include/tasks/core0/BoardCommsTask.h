#pragma once

#include <TaskBase.h>
#include <QueueManager1.h>
#include <tasks/queues/types/root.h>

namespace BoardCommsTask
{
  TaskBase *thisTask;

#ifndef RADIO_OBJECTS

#include <NRF24L01Lib.h>
#include <GenericClient.h>

  NRF24L01Lib nrf24;

  RF24 radio(NRF_CE, NRF_CS);
  RF24Network network(radio);

#endif

  GenericClient<ControllerData, VescData> boardClient(COMMS_BOARD);

  namespace
  {
    Queue1::Manager<PacketState> *packetStateQueue = nullptr;

    ControllerConfig controller_config;
    ControllerData controller_packet;

    BoardClass board;

    PacketState packetState;

    bool printSentPacketToBoard = false;
    unsigned long SEND_TO_BOARD_INTERVAL_LOCAL = SEND_TO_BOARD_INTERVAL;

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
      packetStateQueue->send(&packetState, thisTask->printSendToQueue ? QueueBase::printSend : nullptr);

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

      if (success == false)
        Serial.printf("Unable to send CONTROL packet to board id: %lu\n", controller_packet.id);
    }
    //----------------------------------------------------------
    void initialiseQueues()
    {
      packetStateQueue = Queue1::Manager<PacketState>::create("(BoardCommsTask)PacketStateQueue");
    }
    //----------------------------------------------------------
    void initialise()
    {
      packetStateQueue->read(); // clear the queue

      boardClient.begin(&network, boardPacketAvailable_cb, mux_SPI);
    }
    //----------------------------------------------------------
    elapsedMillis since_checked_for_available, since_sent_to_board = 0;
    elapsedMillis since_last_did_work = 0;

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
    }
    //----------------------------------------------------------
    void task(void *parameters)
    {
      thisTask->task(parameters);
    }
  }

  void start(ulong doWorkInterval, ulong sendToBoardInterval)
  {
    thisTask = new TaskBase("BoardCommsTask", 3000);
    thisTask->setInitialiseCallback(initialise);
    thisTask->setInitialiseQueuesCallback(initialiseQueues);
    thisTask->setTimeToDoWorkCallback(timeToDowork);
    thisTask->setDoWorkCallback(doWork);

    thisTask->doWorkInterval = doWorkInterval;
    SEND_TO_BOARD_INTERVAL_LOCAL = sendToBoardInterval;

    if (thisTask->rtos != nullptr)
      thisTask->rtos->create(task, CORE_0, TASK_PRIORITY_1, WITH_HEALTHCHECK);
  }

  void deleteTask(bool print = false)
  {
    if (thisTask != nullptr && thisTask->rtos != nullptr)
      thisTask->rtos->deleteTask(print);
  }
}