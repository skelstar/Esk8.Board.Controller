
// #define LONGCLICK_MS 1000

#include <Button2.h>

#define BUTTON_PRIMARY 21
Button2 primaryButton(BUTTON_PRIMARY);

bool pulseLedOn = false;

void primaryButtonInit()
{
  pulseLedOn = false;

  primaryButton.setPressedHandler([](Button2 &btn) {
    // Serial.printf("Primary Button pressed\n");
  });
  primaryButton.setClickHandler([](Button2 &btn) {
    // Serial.printf("Primary Button click\n");
    buttonQueueManager->send(ButtonClickType::SINGLE);
  });

  primaryButton.setDoubleClickHandler([](Button2 &btn) {
    // Serial.printf("Primary Button double-clicked\n");
    buttonQueueManager->send(ButtonClickType::DOUBLE);
  });

  primaryButton.setTripleClickHandler([](Button2 &btn) {
    // Serial.printf("Primary Button triple-clicked\n");
    buttonQueueManager->send(ButtonClickType::TRIPLE);
    pulseLedOn = !pulseLedOn;
    hudMessageQueueManager->send(pulseLedOn
                                     ? HUD_EV_PULSE_RED
                                     : HUD_EV_CONNECTED);
  });
}