#pragma once

#include <TaskBase.h>
#include <QueueManager.h>
#include <tasks/queues/QueueFactory.h>
#include <Fsm.h>
#include <FsmManager.h>
#include <Wire.h>
#include <utils.h>
#include <U8g2lib.h>

#define QWIICDISPLAY_TASK

#define RESPONSE_WINDOW 500

namespace nsQwiicDisplayTask
{
  bool m_printDebug = false;

  float g_BoardBatteryVolts = 0.0;

  U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);

  // prototypes
  void centreMessage(const char *message, bool takeMutex = true);
  void valueMessage(const char *left, const char *right, bool takeMutex = true);
  void initialiseQwiicDisplay();
}

#include <tasks/core0/stateMachines/QwiicDisplayStateMachine.h>

class QwiicDisplayTask : public TaskBase
{
public:
  bool printWarnings = true,
       printDebug = false,
       printFsmTrigger = false;

private:
  Queue1::Manager<SimplMessageObj> *simplMsgQueue = nullptr;
  Queue1::Manager<Transaction> *transactionQueue = nullptr;

  SimplMessageObj simplMessage;
  ThrottleState _throttleState;
  Transaction m_transaction;

  uint8_t _offlineCount = 0;
  const int NUM_CONSEQ_OFFLINE_EVENTS = 3;

public:
  QwiicDisplayTask() : TaskBase("QwiicDisplayTask", 3000)
  {
    _core = CORE_0;
  }

  void _initialise()
  {
    using namespace nsQwiicDisplayTask;

    nsQwiicDisplayTask::m_printDebug = printDebug;

    if (mux_I2C == nullptr)
      mux_I2C = xSemaphoreCreateMutex();

    simplMsgQueue = createQueueManager<SimplMessageObj>("(QwiicDisplayTask)simplMsgQueue");
    transactionQueue = createQueueManager<Transaction>("(QwiicDisplayTask)transactionQueue");

    initialiseQwiicDisplay();
    centreMessage("Started");
    // fsm stuff
    fsm_mgr.begin(&_fsm);
    fsm_mgr.setPrintStateCallback(printState);
    fsm_mgr.setPrintTriggerCallback(printTrigger);
    addTransitions();
    _fsm.run_machine();

    _offlineCount = 0;
  }

  void doWork()
  {
    if (simplMsgQueue->hasValue())
      _handleSimplMessage(simplMsgQueue->payload);

    if (transactionQueue->hasValue())
      _handleTransaction(transactionQueue->payload);

    nsQwiicDisplayTask::fsm_mgr.runMachine();
  }

  void cleanup()
  {
    delete (simplMsgQueue);
  }

  void _handleSimplMessage(SimplMessageObj &simplMessage)
  {
  }

  void _handleTransaction(Transaction &payload)
  {
    using namespace nsQwiicDisplayTask;

    bool wasOnline = m_transaction.connected(RESPONSE_WINDOW),
         offline = payload.connected(RESPONSE_WINDOW) == false;

    if (offline)
    {
      _offlineCount++;
      // offline
      if (_offlineCount >= NUM_CONSEQ_OFFLINE_EVENTS)
        fsm_mgr.trigger(EV_OFFLINE);
    }
    else
    {
      _offlineCount = 0;

      g_BoardBatteryVolts = payload.batteryVolts;
      // online
      if (m_transaction.moving != payload.moving ||
          m_transaction.event_id == 0 ||
          payload.reason == FIRST_PACKET)
      {
        if (payload.moving)
        {
          fsm_mgr.trigger(EV_MOVING);
        }
        else
        {
          // stopped
          fsm_mgr.trigger(EV_STOPPED);
        }
      }
    }

    m_transaction = payload;
  }
};

QwiicDisplayTask qwiicDisplayTask;

#define FONT_SIZE_MED u8g2_font_profont22_tr
#define FONT_SIZE_LG u8g2_font_profont29_tr

namespace nsQwiicDisplayTask
{
  void task1(void *parameters)
  {
    qwiicDisplayTask.task(parameters);
  }

  void initialiseQwiicDisplay()
  {
    if (take(mux_I2C, TICKS_500ms, __func__))
    {
      Wire.begin();
      if (u8g2.begin() == false)
      {
        Serial.printf("ERROR: Unable to start QwiicDisplay\n");
      }
      else
      {
        Serial.printf("Initialised QwiicDisplay OK\n");
      }
      give(mux_I2C);
    }
    else
    {
      Serial.printf("[QwiicDisplayTask] Unable to take mux_I2C\n");
    }
  }

  void centreMessage(const char *message, bool takeMutex)
  {
    if (takeMutex || (!takeMutex && take(mux_I2C, TICKS_500ms)))
    {
      u8g2.clearBuffer();
      u8g2.setFont(FONT_SIZE_LG);
      u8g2.setFontPosTop();
      int w = u8g2.getStrWidth(message),
          h = u8g2.getHeight(),
          x = (128 / 2) - (w / 2),
          y = (32 / 2) - (h / 2);
      u8g2.drawStr(x, y, message);
      u8g2.sendBuffer();
      if (takeMutex)
        give(mux_I2C);
    }
    else
    {
      DEBUG("Couldn't take mutex (QwiicDisplayTask centreMessage())");
    }
  }

  void valueMessage(const char *left, const char *right, bool takeMutex)
  {
    if (takeMutex || (!takeMutex && take(mux_I2C, TICKS_500ms)))
    {
      int w = 0, h = 0;

      u8g2.clearBuffer();
      u8g2.setFont(FONT_SIZE_MED);
      u8g2.setFontPosBottom();

      h = u8g2.getHeight();
      // left
      u8g2.drawStr(0, 16 + (h / 2), left);

      w = u8g2.getStrWidth(right);
      u8g2.drawStr(128 - w, 16 + (h / 2), right);
      u8g2.sendBuffer();
      if (takeMutex)
        give(mux_I2C);
    }
    else
    {
      DEBUG("Couldn't take mutex (QwiicDisplayTask valueMessage())");
    }
  }
}