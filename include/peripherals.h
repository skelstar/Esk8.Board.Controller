
#ifdef PRIMARY_BUTTON_PIN
#define PRIMARY_BUTTON_PIN 21
Button2 primaryButton(PRIMARY_BUTTON_PIN);
#endif

#define BUTTON_RIGHT 35
// Button2 rightButton(BUTTON_RIGHT);

void primaryButtonInit()
{
  pulseLedOn = TriState::STATE_NONE;

#ifdef PRIMARY_BUTTON_PIN
  primaryButton.setPressedHandler([](Button2 &btn) {
    if (PRINT_BUTTON_EVENTS)
      DEBUG("Primary Button pressed");
  });

  primaryButton.setClickHandler([](Button2 &btn) {
    if (PRINT_BUTTON_EVENTS)
      DEBUG("Primary Button click");
  });

  primaryButton.setDoubleClickHandler([](Button2 &btn) {
    if (PRINT_BUTTON_EVENTS)
      DEBUG("Primary Button double-clicked");
  });

  primaryButton.setTripleClickHandler([](Button2 &btn) {
    if (PRINT_BUTTON_EVENTS)
      DEBUG("Primary Button triple-clicked");
  });

  primaryButton.setLongClickDetectedHandler([](Button2 &btn) {
    if (PRINT_BUTTON_EVENTS)
      DEBUG("Longpress of primary button detected");
  });
#endif
}
