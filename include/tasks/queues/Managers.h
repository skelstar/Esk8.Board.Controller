#pragma once

#include <QueueManager1.h>

Queue1::Manager<DisplayEvent> *displayEventQueue = nullptr;
Queue1::Manager<PrimaryButtonState> *primaryButtonQueue = nullptr;
Queue1::Manager<PacketState> *packetStateQueue = nullptr;
Queue1::Manager<NintendoButtonEvent> *nintendoQueue = nullptr;
Queue1::Manager<ThrottleState> *throttleQueue = nullptr;
