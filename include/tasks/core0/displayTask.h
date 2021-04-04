#include <statsClass.h>
#include <TFT_eSPI.h>
#include <tft.h>
#include <printFormatStrings.h>

#ifndef PRINT_DISP_STATE
#define PRINT_DISP_STATE 0
#define PRINT_DISP_STATE_EVENT 0
#endif

namespace Display
{
  RTOSTaskManager mgr("DisplayTask", 3000);

  void printState(uint16_t id);
  void printTrigger(uint16_t ev);

  // prototypes
  void handle_stats_packet(StatsClass *res);
  void handle_board_packet(BoardClass *res);
  void handle_nintendo_classic_event(NintendoButtonEvent *ev);

  void task(void *pvParameters)
  {
    mgr.printStarted();

    setupLCD();

    fsm_mgr.begin(&_fsm);
    fsm_mgr.setPrintStateCallback(printState);
    fsm_mgr.setPrintTriggerCallback(printTrigger);

    addTransitions();

    _fsm.run_machine();

    remote = new BatteryInfo();

    elapsedMillis sinceReadDispEventQueue, since_checked_queue, since_fsm_update;
    unsigned long last_board_id = -1, last_classic_id = -1, last_batt_id = -1;

    while (boardPacketQueue == nullptr)
    {
      vTaskDelay(10);
    }

    mgr.ready = true;
    mgr.printReady();

    while (true)
    {
      if (since_checked_queue > 100)
      {
        since_checked_queue = 0;

        BoardClass *board = boardPacketQueue->peek<BoardClass>(__func__);
        if (board != nullptr)
        {
          if (board->id != last_board_id)
          {
            last_board_id = board->id;
            handle_board_packet(board);
          }
          else
          {
          }
        }

        if (NintendoClassicTask::queue != nullptr)
        {
          NintendoButtonEvent *ev = NintendoClassicTask::queue->peek<NintendoButtonEvent>(__func__);
          if (ev != nullptr && ev->event_id != last_classic_id)
          {
            handle_nintendo_classic_event(ev);
            last_classic_id = ev->event_id;
          }
        }

        if (Remote::queue != nullptr)
        {
          BatteryInfo *batt = Remote::queue->peek<BatteryInfo>(__func__);
          if (batt != nullptr && !batt->been_peeked(last_batt_id))
          {
            last_batt_id = batt->event_id;
            remote = new BatteryInfo(*batt);
            fsm_mgr.trigger(DispState::TR_REMOTE_BATTERY_CHANGED);
          }
        }
      }

      if (since_fsm_update > 50)
      {
        since_fsm_update = 0;
        _fsm.run_machine();
      }

      mgr.healthCheck(10000);

      vTaskDelay(10);
    }
    vTaskDelete(NULL);
  }
  //-----------------------------------------------------

  void printState(uint16_t id)
  {
    if (PRINT_DISP_STATE)
      Serial.printf(PRINT_STATE_FORMAT, "DISP", Display::stateID(id));
  }

  void printTrigger(uint16_t ev)
  {
    if (PRINT_DISP_STATE_EVENT &&
        !(_fsm.revisit() && ev == DispState::TR_STOPPED) &&
        !(_fsm.revisit() && ev == DispState::TR_MOVING) &&
        !(_fsm.getCurrentStateId() == Display::ST_OPTION_PUSH_TO_START && ev == DispState::TR_STOPPED))
      Serial.printf(PRINT_sFSM_sTRIGGER_FORMAT, "DISP", DispState::getTrigger(ev));
  }

  void handle_board_packet(BoardClass *board)
  {
    // Serial.printf("Rx packet from board:");
    // Serial.printf("moving=%d:", board->packet.moving);
    // Serial.println();

    if (board->packet.version != (float)VERSION_BOARD_COMPAT &&
        !fsm_mgr.currentStateIs(ST_BOARD_VERSION_DOESNT_MATCH_SCREEN))
    {
      Serial.printf("%.1f %.1f\n", board->packet.version, (float)VERSION_BOARD_COMPAT);
      fsm_mgr.trigger(DispState::TR_VERSION_DOESNT_MATCH);
    }
    else if (board->connected())
    {
      if (board->isMoving())
        fsm_mgr.trigger(DispState::Trigger::TR_MOVING);
      else if (board->isStopped())
        fsm_mgr.trigger(DispState::Trigger::TR_STOPPED);
    }
    else
      // offline
      fsm_mgr.trigger(DispState::TR_DISCONNECTED);
  }

  DispState::Trigger mapToTrigger(uint8_t *buttons)
  {
    if (buttons[NintendoController::BUTTON_START] == NintendoController::BUTTON_PRESSED)
      return DispState::TR_MENU_BUTTON_CLICKED;
    return DispState::TR_NO_EVENT;
  }

  DispState::Trigger mapToTrigger(uint8_t button, uint8_t state)
  {
    switch (button)
    {
    case NintendoController::BUTTON_START:
      return state == 1 ? DispState::TR_MENU_BUTTON_CLICKED : DispState::TR_NO_EVENT; // only on press
    }
    return DispState::TR_NO_EVENT;
  }

  void handle_nintendo_classic_event(NintendoButtonEvent *ev)
  {
    DispState::Trigger tr = mapToTrigger(ev->button, ev->state);
    if (tr != DispState::TR_NO_EVENT)
      fsm_mgr.trigger(tr);
  }
} // namespace Display