
// #define LONGCLICK_MS 1000

#include <Button2.h>

#define BUTTON_PRIMARY 21
Button2 primaryButton(BUTTON_PRIMARY);

void primaryButtonInit()
{
  primaryButton.setPressedHandler([](Button2 &btn) {
    Serial.printf("Primary Button pressed\n");
  });

  primaryButton.setDoubleClickHandler([](Button2 &btn) {
    Serial.printf("Primary Button double-clicked\n");
  });

  primaryButton.setTripleClickHandler([](Button2 &btn) {
    Serial.printf("Primary Button triple-clicked\n");
  });
}