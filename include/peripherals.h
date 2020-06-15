
// #define LONGCLICK_MS 1000

#include <Button2.h>

#define BUTTON_PRIMARY 21
Button2 primaryButton(BUTTON_PRIMARY);

void primaryButtonInit()
{
  primaryButton.setPressedHandler([](Button2 &btn) {
    DEBUG("Primary Button pressed");
  });

  primaryButton.setDoubleClickHandler([](Button2 &btn) {
    DEBUG("Primary Button double-clicked");
  });
}