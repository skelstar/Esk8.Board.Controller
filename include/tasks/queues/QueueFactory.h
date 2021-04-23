#pragma once

#include <tasks/queues/types/PacketState.h>
#include <tasks/queues/types/NintendoButtonEvent.h>
#include <tasks/queues/types/PrimaryButton.h>
#include <tasks/queues/types/Throttle.h>
#include <tasks/queues/types/QueueBase.h>
#include <tasks/queues/types/DisplayEvent.h>

#include <tasks/queues/queues.h>

#include <QueueManager.h>

template <typename T>
Queue1::Manager<T> *createQueue(const char *name, TickType_t ticks = TICKS_5ms)
{
  if (std::is_same<T, BatteryInfo>::value)
  {
    return new Queue1::Manager<T>(xBatteryInfo, TICKS_5ms, name);
  }
  if (std::is_same<T, PrimaryButtonState>::value)
  {
    return new Queue1::Manager<T>(xPrimaryButtonQueueHandle, TICKS_5ms, name);
  }
  if (std::is_same<T, ThrottleState>::value)
  {
    return new Queue1::Manager<T>(xThrottleQueueHandle, TICKS_5ms, name);
  }
  if (std::is_same<T, PacketState>::value)
  {
    return new Queue1::Manager<T>(xPacketStateQueueHandle, TICKS_5ms, name);
  }
  if (std::is_same<T, NintendoButtonEvent>::value)
  {
    return new Queue1::Manager<T>(xNintendoControllerQueue, TICKS_5ms, name);
  }
  if (std::is_same<T, DisplayEvent>::value)
  {
    return new Queue1::Manager<T>(xDisplayQueueHandle, TICKS_5ms, name);
  }
  Serial.printf("ERROR: (Manager::create) a queue has not been created for this type (%s)\n", name);
  return nullptr;
}
