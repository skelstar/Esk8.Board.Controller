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

namespace Comms
{
  /* prototypes */
  void print(const char *stateName);
  void init();

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

  Queue::Manager *queue1;

  //------------------------------------------

  State stateDisconnected(
      [] {
        commsFsm.printState(StateId::DISCONNECTED);
        if (Stats::mutex.take("Comms: stateDisconnected", TICKS_100))
        {
          stats.boardConnected = false;
          Stats::mutex.give("Comms: stateDisconnected");
        }
      },
      NULL,
      NULL);

  State stateConnected(
      [] {
        commsFsm.printState(StateId::CONNECTED);
        if (Board::mutex.take("Comms: stateConnected", TICKS_100))
        {
          bool boardCompatible = boardVersionCompatible(board.packet.version);

          if (Stats::mutex.take("Comms: stateConnected", TICKS_100))
          {
            if (!boardCompatible)
              displayQueue->send(DispState::VERSION_DOESNT_MATCH);
            else if (stats.boardConnectedThisSession)
              stats.boardResets++;

            comms_session_started = true;
            stats.boardConnected = true;
            Stats::mutex.give("Comms: stateConnected");
          }
          Board::mutex.give("Comms: stateConnected");
        }
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
    fsm.add_transition(&stateDisconnected, &stateConnected, Comms::Event::PKT_RXD, NULL);
    fsm.add_transition(&stateDisconnected, &stateConnected, Comms::Event::BOARD_FIRST_PACKET, NULL);

    fsm.add_transition(&stateConnected, &stateConnected, Comms::Event::BOARD_FIRST_PACKET, NULL);

    fsm.add_transition(&stateConnected, &stateDisconnected, Comms::Event::BOARD_TIMEDOUT, NULL);
  }
  //-----------------------------------------------------

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
      if (PRINT_COMMS_STATE_EVENT && ev != 0 && ev != Comms::PKT_RXD)
        Serial.printf(PRINT_sFSM_sTRIGGER_FORMAT, "COMMS", Comms::getEventName(ev));
    });

    Comms::addTransitions();

    fsm.run_machine();

    init();

    taskReady = true;

    while (!Display::taskReady)
    {
      vTaskDelay(1);
    }

    while (true)
    {
      Comms::Event ev = Comms::queue1->read<Comms::Event>();
      if (ev != Comms::NO_EVENT)
        Comms::commsFsm.trigger(ev);
      Comms::fsm.run_machine();

      vTaskDelay(10);
    }
    vTaskDelete(NULL);
  }
  //------------------------------------------------------------

  void queueSent_cb(uint16_t ev)
  {
    if (PRINT_COMMS_QUEUE_SENT)
      Serial.printf(PRINT_QUEUE_SEND_FORMAT, getEventName(ev), "COMMS");
  }

  void queueRead_cb(uint16_t ev)
  {
    if (PRINT_COMMS_QUEUE_READ)
      Serial.printf(PRINT_QUEUE_SEND_FORMAT, getEventName(ev), "COMMS");
  }

  void init()
  {
    queue1 = new Queue::Manager(/*len*/ 3, sizeof(Comms::Event), /*ticks*/ 10);
    queue1->setName("Comms");
    queue1->setSentEventCallback(queueSent_cb);
    queue1->setReadEventCallback(queueRead_cb);
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
