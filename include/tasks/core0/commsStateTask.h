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

/* prototypes */

void print(const char *stateName);

int triggerEvent = CommsState::NO_EVENT;

//------------------------------------------
State stateCommsSearching([] {
  print("stateCommsSearching");
});

State stateCommsConnected(
    [] {
      print("stateCommsConnected");
      if (triggerEvent == CommsState::BOARD_FIRST_PACKET)
      {
        stats.boardResets++;
        displayQueue->send(DispState::UPDATE);
      }

      comms_session_started = true;
      stats.boardConnected = true;

      displayQueue->send(DispState::CONNECTED);
      displayQueue->send(DispState::UPDATE);

      hudMessageQueue->send(HUDTask::BOARD_CONNECTED);

      if (stats.needToAckResets())
      {
        displayQueue->send(DispState::SW_RESET);
        pulseLedOn = TriState::STATE_ON;
        hudMessageQueue->send(HUDTask::CONTROLLER_RESET);
      }

      // check board version is compatible
      bool boardCompatible = boardVersionCompatible(board.packet.version);
      if (!boardCompatible)
      {
        displayQueue->send(DispState::VERSION_DOESNT_MATCH);
      }
    },
    NULL,
    NULL);

State stateCommsDisconnected(
    [] {
      print("stateCommsDisconnected");
      if (triggerEvent == CommsState::BOARD_FIRST_PACKET)
      {
        stats.boardResets++;
        displayQueue->send(DispState::UPDATE);
      }

      stats.boardConnected = false;
      displayQueue->send(DispState::DISCONNECTED);
      hudMessageQueue->send(HUDTask::BOARD_DISCONNECTED);
    },
    NULL, NULL);

Fsm commsFsm(&stateCommsSearching);

//-----------------------------------------------------

void addCommsStateTransitions()
{
  // CommsState::PKT_RXD
  commsFsm.add_transition(&stateCommsSearching, &stateCommsConnected, CommsState::PKT_RXD, NULL);
  commsFsm.add_transition(&stateCommsDisconnected, &stateCommsConnected, CommsState::PKT_RXD, NULL);

  // CommsState::BOARD_TIMEDOUT
  commsFsm.add_transition(&stateCommsConnected, &stateCommsDisconnected, CommsState::BOARD_TIMEDOUT, NULL);

  // CommsState::BD_RESET
  commsFsm.add_transition(&stateCommsConnected, &stateCommsConnected, CommsState::BOARD_FIRST_PACKET, NULL);
  commsFsm.add_transition(&stateCommsDisconnected, &stateCommsDisconnected, CommsState::BOARD_FIRST_PACKET, NULL);
}

//-----------------------------------------------------

void commsStateEventCb(int ev)
{
  triggerEvent = ev;
  // only print if PRINT and not CommsState::PKT_RXD
  if (PRINT_COMMS_STATE_EVENT && !SUPPRESS_EV_COMMS_PKT_RXD || ev != CommsState::PKT_RXD)
    Serial.printf("--> CommsEvent: %s\n", CommsState::names[(int)ev]);
}

void commsStateTask(void *pvParameters)
{

  Serial.printf("commsStateTask running on core %d\n", xPortGetCoreID());

  commsStateTask_initialised = true;

  commsFsm.setEventTriggeredCb(commsStateEventCb);

  addCommsStateTransitions();

  while (false == display_task_initialised)
  {
    vTaskDelay(1);
  }

  while (true)
  {
    CommsState::Event ev = nrfCommsQueue->read<CommsState::Event>();
    if (ev != CommsState::NO_EVENT)
      commsFsm.trigger(ev);
    commsFsm.run_machine();

    vTaskDelay(10);
  }
  vTaskDelete(NULL);
}
//------------------------------------------------------------

void createCommsStateTask_0(uint8_t core, uint8_t priority)
{
  xTaskCreatePinnedToCore(
      commsStateTask,
      "commsStateTask",
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

void print(const char *stateName)
{
  if (PRINT_COMMS_STATE)
  {
    char debugTime[10];
    sprintf(debugTime, "%6.1fs", millis() / 1000.0);
    Serial.printf("[%s] %s\n", debugTime, stateName);
  }
}
