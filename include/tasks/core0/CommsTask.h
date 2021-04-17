#include <TaskBase.h>
#include <QueueManager1.h>
#include <types/SendToBoardNotf.h>
#include <types/PacketState.h>

namespace CommsTask
{
  TaskBase *thisTask;

#ifndef RADIO_OBJECTS
#define RADIO_OBJECTS

#include <RF24Network.h>
#include <NRF24L01Lib.h>

  NRF24L01Lib nrf24;

  RF24 radio(NRF_CE, NRF_CS);
  RF24Network network(radio);
#endif

  namespace
  {
    Queue1::Manager<SendToBoardNotf> *scheduleQueue = nullptr;
    Queue1::Manager<PacketState> *packetStateQueue = nullptr;

    GenericClient<ControllerData, VescData> boardClient(COMMS_BOARD);

    ControllerConfig controller_config;
    ControllerData controller_packet;

    BoardClass board;

    PacketState packetState;

    bool printReplyToSchedule = false,
         printPeekSchedule = false;

    //----------------------------------------------------------
    void boardPacketAvailable_cb(uint16_t from_id, uint8_t t)
    {
      VescData packet = boardClient.read();
      board.save(packet);

      if (packet.reason == CONFIG_RESPONSE)
      {
        Serial.printf("CONFIG_RESPONSE id: %lu\n", packet.id);
      }
      DEBUGMVAL("boardPacketAvailable_cb", board.packet.id, controller_packet.id);
      packetState.received(packet);
      packetStateQueue->reply(&packetState, printReplyToSchedule ? QueueBase::printSend : nullptr);

      vTaskDelay(10);
    }
    //----------------------------------------------------------
    void sendPacketToBoard(bool print = false)
    {
      controller_packet.id++;
      controller_packet.acknowledged = false;

      if (print)
        Serial.printf("sendPacketToBoard() id: %lu\n", controller_packet.id);

      bool success = boardClient.sendTo(Packet::CONTROL, controller_packet);

      packetState.sent(controller_packet);

      if (success == false)
        Serial.printf("Unable to send CONTROL packet to board id: %lu\n", controller_packet.id);
    }

    void initialiseQueues()
    {
      scheduleQueue = Queue1::Manager<SendToBoardNotf>::create("(CommsTask)NotfQueue");
      packetStateQueue = Queue1::Manager<PacketState>::create("(CommsTask)PacketStateQueue");
    }

    void initialise()
    {
      packetState.correlationId = -1;

      boardClient.begin(&network, boardPacketAvailable_cb, mux_SPI);
    }

    elapsedMillis since_checked_for_available, since_sent_to_board = 0;

    bool timeToDowork()
    {
      uint8_t status = waitForNew(scheduleQueue, PERIOD_50ms, printPeekSchedule ? QueueBase::printRead : nullptr);
      if (status == Response::OK)
      {
        packetState.correlationId = scheduleQueue->payload.correlationId;
        packetState.sent_time = scheduleQueue->payload.sent_time;
        return true;
      }

      if (since_checked_for_available > PERIOD_50ms)
      {
        return true;
      }
      return false;
    }

    void doWork()
    {
      if (packetStateQueue == nullptr)
      {
        Serial.printf("ERROR: packetStateQueue is NULL\n");
        return;
      }

      // need to work out if we have a new packet to send or not (in case board doesnt respond)

      sendPacketToBoard(PRINT_THIS);

      DEBUGMVAL("sent to board (doWork)", controller_packet.id, board.packet.id);

      bool responded = false;
      while (since_sent_to_board < PERIOD_100ms && responded == false)
      {
        since_checked_for_available = 0;

        boardClient.update();
        vTaskDelay(10);
      }
      DEBUGMVAL("doneWork?()", controller_packet.id, board.packet.id);
      packetStateQueue->reply(&packetState, printReplyToSchedule ? QueueBase::printSend : nullptr);
    }

    void task(void *parameters)
    {
      thisTask->task(parameters);
    }
  }

  void start()
  {
    thisTask = new TaskBase("CommsTask", 3000);
    thisTask->setInitialiseCallback(initialise);
    thisTask->setInitialiseQueuesCallback(initialiseQueues);
    thisTask->setTimeToDoWorkCallback(timeToDowork);
    thisTask->setDoWorkCallback(doWork);
    thisTask->enabled = true;

    if (thisTask->rtos != nullptr)
      thisTask->rtos->create(task, CORE_0, TASK_PRIORITY_1, WITH_HEALTHCHECK);
  }

  void deleteTask(bool print = false)
  {
    if (thisTask != nullptr && thisTask->rtos != nullptr)
      thisTask->rtos->deleteTask(print);
  }
}