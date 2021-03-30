#include <statsClass.h>
#include <TFT_eSPI.h>
#include <tft.h>

namespace Display
{
  void printState(uint16_t id);
  void printTrigger(uint16_t ev);

  StatsClass *_stats;
  BoardClass *_board;
  nsPeripherals::Peripherals *_periphs;

  // prototypes
  void handle_stats_packet(StatsClass *res);
  void handle_board_packet(BoardClass *res);
  void handle_peripherals_packet(nsPeripherals::Peripherals *res);
  void handle_nintendo_classic_event(NintendoButtonEvent *ev);

  bool taskReady = false;

  void task(void *pvParameters)
  {
    Serial.printf(PRINT_TASK_STARTED_FORMAT, "Display", xPortGetCoreID());

    setupLCD();

    fsm_mgr.begin(&_fsm);
    fsm_mgr.setPrintStateCallback(printState);
    fsm_mgr.setPrintTriggerCallback(printTrigger);

    addTransitions();

    _fsm.run_machine();

    taskReady = true;

    _stats = new StatsClass();
    _board = new BoardClass();
    _periphs = new nsPeripherals::Peripherals();

    elapsedMillis sinceReadDispEventQueue, since_checked_queue, since_fsm_update;
    unsigned long last_board_id = -1, last_periph_id = -1, last_classic_id = -1;

    while (true)
    {
      if (since_checked_queue > SEND_TO_BOARD_INTERVAL)
      {
        since_checked_queue = 0;

        BoardClass *_brd_res = boardPacketQueue->peek<BoardClass>(__func__);
        if (_brd_res != nullptr && last_board_id != _brd_res->id)
        {
          handle_board_packet(_brd_res);
          last_board_id = _brd_res->id;
        }

        nsPeripherals::Peripherals *_periph_res = peripheralsQueue->peek<nsPeripherals::Peripherals>(__func__);
        if (_periph_res != nullptr && last_periph_id != _periph_res->id)
        {
          handle_peripherals_packet(_periph_res);
          last_periph_id = _periph_res->id;
        }
        NintendoButtonEvent *ev = NintendoClassicTask::queue->peek<NintendoButtonEvent>(__func__);
        if (ev != nullptr && ev->id != last_classic_id)
        {
          handle_nintendo_classic_event(ev);
          last_classic_id = ev->id;
        }
      }

      if (since_fsm_update > 50)
      {
        since_fsm_update = 0;
        _fsm.run_machine();
      }

      vTaskDelay(10);
    }
    vTaskDelete(NULL);
  }
  //-----------------------------------------------------

  void createTask(uint8_t core, uint8_t priority)
  {
    xTaskCreatePinnedToCore(
        task,
        "displayTask",
        10000,
        NULL,
        priority,
        NULL,
        core);
  }

  void printState(uint16_t id)
  {
    if (PRINT_DISP_STATE)
      Serial.printf(PRINT_STATE_FORMAT, "DISP", Display::stateID(id));
  }

  void printTrigger(uint16_t ev)
  {
    if (PRINT_DISP_STATE_EVENT &&
        !(_fsm.revisit() && ev == DispState::STOPPED) &&
        !(_fsm.revisit() && ev == DispState::MOVING) &&
        !(_fsm.getCurrentStateId() == Display::OPTION_PUSH_TO_START && ev == DispState::STOPPED))
      Serial.printf(PRINT_sFSM_sTRIGGER_FORMAT, "DISP", DispState::getTrigger(ev));
  }

  void handle_board_packet(BoardClass *board)
  {
    if (board->packet.version != (float)VERSION_BOARD_COMPAT &&
        !fsm_mgr.currentStateIs(BOARD_VERSION_DOESNT_MATCH_SCREEN))
    {
      fsm_mgr.trigger(DispState::VERSION_DOESNT_MATCH);
    }
    else if (board->connected())
    {
      if (!fsm_mgr.currentStateIs(DispState::MOVING) && board->isMoving())
        fsm_mgr.trigger(DispState::MOVING);
      else if (!fsm_mgr.currentStateIs(DispState::STOPPED) && board->isStopped())
        fsm_mgr.trigger(DispState::STOPPED);
    }
    else
      // offline
      fsm_mgr.trigger(DispState::DISCONNECTED);
  }

  DispState::Trigger mapToTrigger(uint8_t *buttons)
  {
    if (buttons[NintendoController::BUTTON_START] == NintendoController::BUTTON_PRESSED)
      return DispState::MENU_BUTTON_CLICKED;
    return DispState::NO_EVENT;
  }

  DispState::Trigger mapToTrigger(uint8_t button, uint8_t state)
  {
    switch (button)
    {
    case NintendoController::BUTTON_START:
      return state == 1 ? DispState::MENU_BUTTON_CLICKED : DispState::NO_EVENT; // only on press
    }
    return DispState::NO_EVENT;
  }

  void handle_peripherals_packet(nsPeripherals::Peripherals *res)
  {
    switch (res->event)
    {
    case nsPeripherals::EV_PRIMARY_BUTTON:
    {
      if (res->primary_button == 1)
        fsm_mgr.trigger(DispState::SELECT_BUTTON_CLICK);
      break;
    }
    }
  }

  void handle_nintendo_classic_event(NintendoButtonEvent *ev)
  {
    DispState::Trigger tr = mapToTrigger(ev->button, ev->state);
    if (tr != DispState::NO_EVENT)
      fsm_mgr.trigger(tr);
  }
} // namespace Display