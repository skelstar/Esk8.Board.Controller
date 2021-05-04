#pragma once

#include <tasks/queues/types/root.h>
#include <QueueManager.h>

Queue1::Manager<DisplayEvent> *displayEventQueue = nullptr;
Queue1::Manager<PrimaryButtonState> *primaryButtonQueue = nullptr;
Queue1::Manager<Transaction> *transactionQueue = nullptr;
Queue1::Manager<NintendoButtonEvent> *nintendoQueue = nullptr;
Queue1::Manager<BatteryInfo> *remoteBatteryQueue = nullptr;
Queue1::Manager<ThrottleState> *throttleQueue = nullptr;
