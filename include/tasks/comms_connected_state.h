#ifndef Fsm
#include <Fsm.h>
#endif

//------------------------------------------

/* prototypes */
bool boardVersionCompatible(float version);
//------------------------------------------

bool comms_session_started = false;
bool commsStateTask_initialised = false;

bool skipOnEnter = false;

CommsState::Event lastCommsEvent = CommsState::NO_EVENT;

Fsm *commsFsm;

/* prototypes */

//------------------------------------------
State stateCommsSearching([] {
  if (PRINT_COMMS_STATE)
    commsFsm->print("stateCommsSearching");
});

State stateCommsConnected(
    [] {
      if (PRINT_COMMS_STATE)
        commsFsm->print("stateCommsConnected");
      if (commsFsm->lastEvent() == CommsState::BD_FIRST_PACKET)
      {
        stats.boardResets++;
        displayChangeQueueManager->send(DispState::UPDATE);
      }

      comms_session_started = true;
      stats.boardConnected = true;

      displayChangeQueueManager->send(DispState::CONNECTED);
      displayChangeQueueManager->send(DispState::UPDATE);

      hudMessageQueue->send(HUDCommand::HEARTBEAT);

      if (stats.needToAckResets())
      {
        displayChangeQueueManager->send(DispState::SW_RESET);
        pulseLedOn = TriState::STATE_ON;
        hudMessageQueue->send(HUDCommand::PULSE_RED);
      }

      // check board version is compatible
      bool boardCompatible = boardVersionCompatible(board.packet.version);
      if (!boardCompatible)
      {
        displayChangeQueueManager->send(DispState::VERSION_DOESNT_MATCH);
      }
    },
    NULL,
    NULL);

State stateCommsDisconnected(
    [] {
      if (PRINT_COMMS_STATE)
      {
        commsFsm->print("stateCommsDisconnected");
      }
      if (commsFsm->lastEvent() == CommsState::BD_FIRST_PACKET)
      {
        stats.boardResets++;
        displayChangeQueueManager->send(DispState::UPDATE);
      }

      stats.boardConnected = false;
      displayChangeQueueManager->send(DispState::DISCONNECTED);
      hudMessageQueue->send(HUDCommand::SPIN_GREEN);
    },
    NULL, NULL);
//-----------------------------------------------------

void addCommsStateTransitions()
{
  // CommsState::PKT_RXD
  commsFsm->add_transition(&stateCommsSearching, &stateCommsConnected, CommsState::PKT_RXD, NULL);
  commsFsm->add_transition(&stateCommsDisconnected, &stateCommsConnected, CommsState::PKT_RXD, NULL);

  // CommsState::BOARD_TIMEDOUT
  commsFsm->add_transition(&stateCommsConnected, &stateCommsDisconnected, CommsState::BOARD_TIMEDOUT, NULL);

  // CommsState::BD_RESET
  commsFsm->add_transition(&stateCommsConnected, &stateCommsConnected, CommsState::BD_FIRST_PACKET, NULL);
  commsFsm->add_transition(&stateCommsDisconnected, &stateCommsDisconnected, CommsState::BD_FIRST_PACKET, NULL);
}

//-----------------------------------------------------

void commsStateEventCb(int ev)
{
  // only print if PRINT and not CommsState::PKT_RXD
  if (PRINT_COMMS_STATE_EVENT && !SUPPRESS_EV_COMMS_PKT_RXD || ev != CommsState::PKT_RXD)
    Serial.printf("--> CommsEvent: %s\n", CommsState::names[(int)ev]);
}

void commsStateTask_0(void *pvParameters)
{

  Serial.printf("commsStateTask_0 running on core %d\n", xPortGetCoreID());

  commsStateTask_initialised = true;

  commsFsm = new Fsm(&stateCommsSearching);
  commsFsm->setEventTriggeredCb(commsStateEventCb);

  addCommsStateTransitions();

  while (false == display_task_initialised)
  {
    vTaskDelay(1);
  }

  while (true)
  {
    CommsState::Event ev = (CommsState::Event)nrfCommsQueueManager->read();
    if (ev != NO_QUEUE_EVENT)
    {
      commsFsm->trigger(ev);
    }
    commsFsm->run_machine();

    vTaskDelay(10);
  }
  vTaskDelete(NULL);
}

void createCommsStateTask_0(uint8_t core, uint8_t priority)
{
  xTaskCreatePinnedToCore(
      commsStateTask_0,
      "commsStateTask_0",
      10000,
      NULL,
      priority,
      NULL,
      core);
}
//------------------------------------------------------------

// check version
bool boardVersionCompatible(float version)
{
  return version == (float)VERSION_BOARD_COMPAT;
}
