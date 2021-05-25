#pragma once

QueueHandle_t xBatteryInfo;
QueueHandle_t xNintendoControllerQueue;
QueueHandle_t xPacketStateQueueHandle;
QueueHandle_t xPrimaryButtonQueueHandle;
QueueHandle_t xSendToBoardQueueHandle;
QueueHandle_t xThrottleQueueHandle;
QueueHandle_t xDisplayQueueHandle;

enum SimplMessage
{
  SIMPL_NONE = 0,
  SIMPL_HEADLIGHT_ON,
  SIMPL_HEADLIGHT_OFF,
};
