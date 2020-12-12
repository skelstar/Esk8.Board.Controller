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
  enum StateId
  {
    SEARCHING,
    CONNECTED,
    DISCONNECTED,
    StateIdLength,
  };

  const char *getStateName(uint8_t id)
  {
    switch (id)
    {
    case SEARCHING:
      return "SEARCHING";
    case CONNECTED:
      return "CONNECTED";
    case DISCONNECTED:
      return "DISCONNECTED";
    }
    return OUT_OF_RANGE;
  }

  FsmManager<Event> commsFsm;

  //------------------------------------------
  State stateCommsSearching([] {
    commsFsm.printState(StateId::SEARCHING);
  });

  State stateCommsConnected(
      [] {
        commsFsm.printState(StateId::CONNECTED);
        if (triggerEvent == Comms::Event::BOARD_FIRST_PACKET)
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
        commsFsm.printState(StateId::DISCONNECTED);
        if (triggerEvent == Comms::Event::BOARD_FIRST_PACKET)
        {
          stats.boardResets++;
          displayQueue->send(DispState::UPDATE);
        }

        stats.boardConnected = false;
        displayQueue->send(DispState::DISCONNECTED);
        hudMessageQueue->send(HUDTask::BOARD_DISCONNECTED);
      },
      NULL, NULL);

  namespace
  {
    Fsm fsm(&stateCommsSearching);
  }
  //-----------------------------------------------------

  void addTransitions()
  {
    // Comms::PKT_RXD
    fsm.add_transition(&stateCommsSearching, &stateCommsConnected, Comms::Event::PKT_RXD, NULL);
    fsm.add_transition(&stateCommsDisconnected, &stateCommsConnected, Comms::Event::PKT_RXD, NULL);

    // Comms::BOARD_TIMEDOUT
    fsm.add_transition(&stateCommsConnected, &stateCommsDisconnected, Comms::Event::BOARD_TIMEDOUT, NULL);

    // Comms::BD_RESET
    fsm.add_transition(&stateCommsConnected, &stateCommsConnected, Comms::Event::BOARD_FIRST_PACKET, NULL);
    fsm.add_transition(&stateCommsDisconnected, &stateCommsDisconnected, Comms::Event::BOARD_FIRST_PACKET, NULL);
  }

  void task(void *pvParameters)
  {

    Serial.printf("commsStateTask running on core %d\n", xPortGetCoreID());

    commsStateTask_initialised = true;

    Comms::commsFsm.begin(&Comms::fsm, STATE_STRING_FORMAT);
    Comms::commsFsm.setGetStateNameCallback([](uint8_t ev) {
      return Comms::getStateName(ev); // Comms::getStateNameSafely(ev);
    });
    Comms::commsFsm.setGetEventNameCallback([](uint8_t ev) {
      return Comms::getEventName(ev);
    });

    Comms::addTransitions();

    while (false == display_task_initialised)
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
