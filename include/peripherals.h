
// #define LONGCLICK_MS 1000

#include <Button2.h>

#define BUTTON_PRIMARY 21
Button2 primaryButton(BUTTON_PRIMARY);

void primaryButtonInit()
{
  pulseLedOn = TriState::STATE_NONE;

  primaryButton.setPressedHandler([](Button2 &btn) {
    if (PRINT_BUTTON_EVENTS)
      DEBUG("Primary Button pressed");
  });
  primaryButton.setClickHandler([](Button2 &btn) {
    if (PRINT_BUTTON_EVENTS)
      DEBUG("Primary Button click");
    buttonQueueManager->send(ButtonClickType::SINGLE);
  });

  primaryButton.setDoubleClickHandler([](Button2 &btn) {
    if (PRINT_BUTTON_EVENTS)
      DEBUG("Primary Button double-clicked");
    buttonQueueManager->send(ButtonClickType::DOUBLE);
    // hud
    pulseLedOn = pulseLedOn == STATE_NONE
                     ? STATE_OFF
                     : pulseLedOn == STATE_OFF
                           ? STATE_ON
                           : STATE_OFF;
    hudMessageQueueManager->send(pulseLedOn
                                     ? HUD_EV_PULSE_RED
                                     : HUD_EV_CONNECTED);
  });

  primaryButton.setTripleClickHandler([](Button2 &btn) {
    if (PRINT_BUTTON_EVENTS)
      DEBUG("Primary Button triple-clicked");
    buttonQueueManager->send(ButtonClickType::TRIPLE);
  });
}