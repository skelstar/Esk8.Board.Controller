#ifndef Fsm
#include <Fsm.h>
#endif
#ifndef FSMMANAGER_H
#include <FsmManager.h>
#endif

//------------------------------------------

bool comms_session_started = false;
bool commsStateTask_initialised = false;

bool skipOnEnter = false;

namespace Comms
{
  RTOSTaskManager mgr("CommsTask", 3000, TASK_PRIORITY_2);

  /* prototypes */
  void print(const char *stateName);
  void init();

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

  QueueHandle_t commsQueue = NULL;
  Queue::Manager *queue1;

  elapsedMillis since_peeked;

  //------------------------------------------

  State stDisconnected(
      [] {
        commsFsm.printState(StateId::DISCONNECTED);
        stats.boardConnected = false;
      },
      NULL,
      NULL);

  State stConnected(
      [] {
        commsFsm.printState(StateId::CONNECTED);

        comms_session_started = true;
        stats.boardConnected = true;
      },
      NULL,
      NULL);

  namespace
  {
    Fsm fsm(&stDisconnected);
  }
  //-----------------------------------------------------

  void addTransitions()
  {
    fsm.add_transition(&stDisconnected, &stConnected, Comms::Event::PKT_RXD, NULL);
    fsm.add_transition(&stDisconnected, &stConnected, Comms::Event::BOARD_FIRST_PACKET, NULL);

    fsm.add_transition(&stConnected, &stConnected, Comms::Event::BOARD_FIRST_PACKET, NULL);

    fsm.add_transition(&stConnected, &stDisconnected, Comms::Event::BOARD_TIMEDOUT, NULL);
  }
  //=====================================================

  void task(void *pvParameters)
  {
    mgr.printStarted();

    commsStateTask_initialised = true;

    commsFsm.begin(&fsm);
    commsFsm.setPrintStateCallback([](uint16_t id) {
      if (PRINT_COMMS_STATE)
        Serial.printf(PRINT_STATE_FORMAT, "COMMS", getStateName(id));
    });
    commsFsm.setPrintTriggerCallback([](uint16_t ev) {
      if (PRINT_COMMS_STATE_EVENT && ev != 0 && ev != PKT_RXD)
        Serial.printf(PRINT_sFSM_sTRIGGER_FORMAT, "COMMS", getEventName(ev));
    });

    addTransitions();

    fsm.run_machine();

    init();

    mgr.ready = true;

#if USING_DISPLAY
    // wait for display task to be ready
    // before checking for packets
    while (!Display::mgr.ready)
    {
      vTaskDelay(1);
    }
#endif

    mgr.printReady();

    while (true)
    {
      if (since_peeked > SEND_TO_BOARD_INTERVAL)
      {
        since_peeked = 0;

        BoardClass *board = boardPacketQueue->peek<BoardClass>(__func__);
        if (board != nullptr)
        {
          if (board->connected())
          {
            if (board->packet.reason == ReasonType::FIRST_PACKET)
            {
              commsFsm.trigger(Event::BOARD_FIRST_PACKET);
            }
            else
              commsFsm.trigger(Event::PKT_RXD);
          }
          else
            commsFsm.trigger(Event::BOARD_TIMEDOUT);
        }
        commsFsm.runMachine();
      }

      mgr.healthCheck(10000);

      vTaskDelay(10);
    }
    vTaskDelete(NULL);
  }
  //------------------------------------------------------------

  void init()
  {
  }
} // namespace Comms

//------------------------------------------------------------

void print(const char *stateName)
{
  if (PRINT_COMMS_STATE)
  {
    char debugTime[10];
    sprintf(debugTime, "%6.1fs", millis() / 1000.0);
    Serial.printf("[%s] %s\n", debugTime, stateName);
  }
}
