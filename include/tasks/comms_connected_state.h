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

CommsStateEvent lastCommsEvent = EV_COMMS_NO_EVENT;

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
      if (commsFsm->lastEvent() == EV_COMMS_BD_FIRST_PACKET)
      {
        stats.boardResets++;
        displayChangeQueueManager->send(DISP_EV_UPDATE);
      }

      comms_session_started = true;
      stats.boardConnected = true;

      displayChangeQueueManager->send(DISP_EV_CONNECTED);
      displayChangeQueueManager->send(DISP_EV_UPDATE);

      hudMessageQueueManager->send(HUD_CMD_HEARTBEAT);

      if (stats.needToAckResets())
      {
        displayChangeQueueManager->send(DISP_EV_SW_RESET);
        pulseLedOn = TriState::STATE_ON;
        hudMessageQueueManager->send(HUD_CMD_PULSE_RED);
      }

      // check board version is compatible
      bool boardCompatible = boardVersionCompatible(board.packet.version);
      if (!boardCompatible)
      {
        displayChangeQueueManager->send(DISP_EV_VERSION_DOESNT_MATCH);
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
      if (commsFsm->lastEvent() == EV_COMMS_BD_FIRST_PACKET)
      {
        stats.boardResets++;
        displayChangeQueueManager->send(DISP_EV_UPDATE);
      }

      stats.boardConnected = false;
      displayChangeQueueManager->send(DISP_EV_DISCONNECTED);
      hudMessageQueueManager->send(HUD_CMD_SPIN_GREEN);
    },
    NULL, NULL);
//-----------------------------------------------------

void addCommsStateTransitions()
{
  // EV_COMMS_PKT_RXD
  commsFsm->add_transition(&stateCommsSearching, &stateCommsConnected, EV_COMMS_PKT_RXD, NULL);
  commsFsm->add_transition(&stateCommsDisconnected, &stateCommsConnected, EV_COMMS_PKT_RXD, NULL);

  // EV_COMMS_BOARD_TIMEDOUT
  commsFsm->add_transition(&stateCommsConnected, &stateCommsDisconnected, EV_COMMS_BOARD_TIMEDOUT, NULL);

  // EV_COMMS_BD_RESET
  commsFsm->add_transition(&stateCommsConnected, &stateCommsConnected, EV_COMMS_BD_FIRST_PACKET, NULL);
  commsFsm->add_transition(&stateCommsDisconnected, &stateCommsDisconnected, EV_COMMS_BD_FIRST_PACKET, NULL);
}

//-----------------------------------------------------

void commsStateEventCb(int ev)
{
  // only print if PRINT and not EV_COMMS_PKT_RXD
  if (PRINT_COMMS_STATE_EVENT && !SUPPRESS_EV_COMMS_PKT_RXD || ev != EV_COMMS_PKT_RXD)
    Serial.printf("--> CommsEvent: %s\n", commsStateEventNames[(int)ev]);
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
    CommsStateEvent ev = (CommsStateEvent)nrfCommsQueueManager->read();
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
