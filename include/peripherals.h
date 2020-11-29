
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
    buttonQueue->send(ButtonClickType::SINGLE);
  });

  primaryButton.setDoubleClickHandler([](Button2 &btn) {
    if (PRINT_BUTTON_EVENTS)
      DEBUG("Primary Button double-clicked");
  });

  primaryButton.setTripleClickHandler([](Button2 &btn) {
    if (PRINT_BUTTON_EVENTS)
      DEBUG("Primary Button triple-clicked");
    buttonQueue->send(ButtonClickType::TRIPLE);
  });
}