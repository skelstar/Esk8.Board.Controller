

#define BUTTON_PRIMARY 21
Button2 primaryButton(BUTTON_PRIMARY);

#define BUTTON_RIGHT 35
Button2 rightButton(BUTTON_RIGHT);

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
    buttonQueue->send(ButtonClickType::DOUBLE);
  });

  primaryButton.setTripleClickHandler([](Button2 &btn) {
    if (PRINT_BUTTON_EVENTS)
      DEBUG("Primary Button triple-clicked");
    buttonQueue->send(ButtonClickType::TRIPLE);
  });

  primaryButton.setLongClickDetectedHandler([](Button2 &btn) {
    if (PRINT_BUTTON_EVENTS)
      DEBUG("Longpress of primary button detected");
    buttonQueue->send(ButtonClickType::LONG_PRESS);
  });
}

void rightButtonInit()
{
  rightButton.setClickHandler([](Button2 &btn) {
    if (PRINT_BUTTON_EVENTS)
      DEBUG("Right Button click");
    displayQueue->send(DispState::RIGHT_BUTTON_CLICKED);
    buttonQueue->send(ButtonClickType::SINGLE);
  });
}