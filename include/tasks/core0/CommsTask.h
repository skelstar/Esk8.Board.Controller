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
      packetState.correlationId = SendToBoardNotf::NO_CORRELATION;
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
      scheduleQueue = Queue1::Manager<SendToBoardNotf>::create("(CommsTask)NotfQueue");
      packetStateQueue = Queue1::Manager<PacketState>::create("(CommsTask)PacketStateQueue");
    }
    //----------------------------------------------------------
    void initialise()
    {
      if (packetStateQueue == nullptr || scheduleQueue == nullptr)
      {
        Serial.printf("ERROR: packetStateQueue or schduleQueue is NULL\n");
        return;
      }

      packetState.correlationId = -1;

      boardClient.begin(&network, boardPacketAvailable_cb, mux_SPI);
    }
    //----------------------------------------------------------
    elapsedMillis since_checked_for_available, since_sent_to_board = 0;
    elapsedMillis since_last_did_work = 0;

    bool timeToDowork()
    {
      return since_last_did_work > PERIOD_50ms && thisTask->enabled;
    }
    //----------------------------------------------------------
    void doWork()
    {
      since_last_did_work = 0;

      boardClient.update();

      // resonse request from Orchestrator
      uint8_t status = waitForNew(scheduleQueue, PERIOD_50ms, thisTask->printPeekSchedule ? QueueBase::printRead : nullptr);
      if (status == Response::OK && scheduleQueue->payload.command == QueueBase::RESPOND)
      {
        packetState.correlationId = scheduleQueue->payload.correlationId;
        packetState.sent_time = scheduleQueue->payload.sent_time;
        packetStateQueue->reply(&packetState, thisTask->printReplyToSchedule ? QueueBase::printReply : nullptr);
      }

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

  void start()
  {
    thisTask = new TaskBase("CommsTask", 3000);
    thisTask->setInitialiseCallback(initialise);
    thisTask->setInitialiseQueuesCallback(initialiseQueues);
    thisTask->setTimeToDoWorkCallback(timeToDowork);
    thisTask->setDoWorkCallback(doWork);

    if (thisTask->rtos != nullptr)
      thisTask->rtos->create(task, CORE_0, TASK_PRIORITY_1, WITH_HEALTHCHECK);
  }

  void deleteTask(bool print = false)
  {
    if (thisTask != nullptr && thisTask->rtos != nullptr)
      thisTask->rtos->deleteTask(print);
  }
}