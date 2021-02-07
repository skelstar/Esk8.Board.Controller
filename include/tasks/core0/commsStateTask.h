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

/* prototypes */

void print(const char *stateName);

int triggerEvent = Comms::Event::NO_EVENT;
Comms::Event lastCommsEvent = Comms::Event::NO_EVENT;

namespace Comms
{
  bool taskReady = false;

  enum StateId
  {
    CONNECTED,
    DISCONNECTED,
    StateIdLength,
  };

  const char *getStateName(uint16_t id)
  {
    switch (id)
    {
    case CONNECTED:
      return "CONNECTED";
    case DISCONNECTED:
      return "DISCONNECTED";
    }
    return OUT_OF_RANGE;
  }

  FsmManager<Event> commsFsm;

  //------------------------------------------

  State stateDisconnected(
      [] {
        commsFsm.printState(StateId::DISCONNECTED);
        stats.boardConnected = false;

        if (lastCommsEvent == Comms::Event::BOARD_TIMEDOUT)
          displayQueue->send(DispState::DISCONNECTED);
        else
          displayQueue->send(DispState::DISCONNECTED);
      },
      NULL,
      NULL);

  State stateConnected(
      [] {
        commsFsm.printState(StateId::CONNECTED);
        bool boardCompatible = boardVersionCompatible(board.packet.version);

        if (stats.boardConnected && triggerEvent == Comms::Event::BOARD_FIRST_PACKET)
        {
          stats.boardResets++;
        }
        else if (!boardCompatible)
          displayQueue->send(DispState::VERSION_DOESNT_MATCH);
        else if (stats.wasUnintendedControllerReset())
          displayQueue->send(DispState::UNINTENDED_RESET);
        else
          displayQueue->send(DispState::CONNECTED);

        comms_session_started = true;
        stats.boardConnected = true;
        stats.boardConnectedThisSession = true;
      },
      NULL,
      NULL);

  namespace
  {
    Fsm fsm(&stateDisconnected);
  }
  //-----------------------------------------------------

  void addTransitions()
  {
    // Comms::PKT_RXD
    fsm.add_transition(&stateDisconnected, &stateConnected, Comms::Event::PKT_RXD, NULL);

    // Comms::BOARD_TIMEDOUT
    fsm.add_transition(&stateConnected, &stateDisconnected, Comms::Event::BOARD_TIMEDOUT, NULL);

    // Comms::BD_RESET
    fsm.add_transition(&stateConnected, &stateConnected, Comms::Event::BOARD_FIRST_PACKET, NULL);
    fsm.add_transition(&stateDisconnected, &stateDisconnected, Comms::Event::BOARD_FIRST_PACKET, NULL);
  }

  void task(void *pvParameters)
  {
    Serial.printf(PRINT_TASK_STARTED_FORMAT, "Comms State", xPortGetCoreID());

    commsStateTask_initialised = true;

    Comms::commsFsm.begin(&Comms::fsm);
    Comms::commsFsm.setPrintStateCallback([](uint16_t id) {
      if (PRINT_COMMS_STATE)
        Serial.printf(PRINT_STATE_FORMAT, "COMMS", Comms::getStateName(id));
    });
    Comms::commsFsm.setPrintTriggerCallback([](uint16_t ev) {
      if (PRINT_COMMS_STATE && ev != 0 && ev != Comms::PKT_RXD)
        Serial.printf(PRINT_sFSM_sTRIGGER_FORMAT, "COMMS", Comms::getEventName(ev));
    });

    Comms::addTransitions();

    taskReady = true;

    while (!Display::taskReady)
    {
      vTaskDelay(1);
    }

    while (true)
    {
      Comms::Event ev = nrfCommsQueue->read<Comms::Event>();
      if (ev != Comms::NO_EVENT)
      {
        bool print = PRINT_COMMS_STATE_EVENT && Comms::commsFsm.lastEvent() != ev; // (ev == Comms::PKT_RXD && !SUPPRESS_EV_COMMS_PKT_RXD);
        Comms::commsFsm.trigger(ev, print);
      }
      Comms::fsm.run_machine();

      vTaskDelay(10);
    }
    vTaskDelete(NULL);
  }
  //------------------------------------------------------------

  void createTask(uint8_t core, uint8_t priority)
  {
    xTaskCreatePinnedToCore(
        task,
        "commsStateTask",
        10000,
        NULL,
        priority,
        NULL,
        core);
  }
} // namespace Comms

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
