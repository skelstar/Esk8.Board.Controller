#ifndef Fsm
#include <Fsm.h>
#endif
#ifndef FSMMANAGER_H
#include <FsmManager.h>
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

  QueueHandle_t commsQueue = NULL;
  Queue::Manager *queue1;

  elapsedMillis since_peeked;

  //------------------------------------------

  State stateDisconnected(
      [] {
        commsFsm.printState(StateId::DISCONNECTED);
        stats.boardConnected = false;
      },
      NULL,
      NULL);

  State stateConnected(
      [] {
        commsFsm.printState(StateId::CONNECTED);
        bool boardCompatible = boardVersionCompatible(board.packet.version);

        if (!boardCompatible)
          displayQueue->send(DispState::VERSION_DOESNT_MATCH);
        else if (stats.boardConnectedThisSession)
          stats.boardResets++;

        comms_session_started = true;
        stats.boardConnected = true;
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

#if OPTION_USING_DISPLAY
    while (!Display::taskReady)
    {
      vTaskDelay(1);
    }
#endif

    while (true)
    {
      if (since_peeked > 500)
      {
        since_peeked = 0;

        if (boardPacketQueue != NULL)
        {
          BoardClass *res = boardPacketQueue->peek<BoardClass>();
          if (res != nullptr)
          {
            if (false == res->hasTimedout())
            {
              if (res->packet.reason == ReasonType::FIRST_PACKET)
                Comms::commsFsm.trigger(Event::BOARD_FIRST_PACKET);
              else
                Comms::commsFsm.trigger(Event::PKT_RXD);
            }
            else
              Comms::commsFsm.trigger(Event::BOARD_TIMEDOUT);
          }
          Comms::commsFsm.runMachine();
        }
      }

      // Comms::Event ev = Comms::queue1->read<Comms::Event>();
      // if (ev != Comms::NO_EVENT)
      //   Comms::commsFsm.trigger(ev);
      // Comms::fsm.run_machine();

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
    commsQueue = xQueueCreate(/*len*/ 3, sizeof(Comms::Event));
    queue1 = new Queue::Manager(commsQueue, (TickType_t)10);
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
