

#ifndef Button2
#include <Button2.h>
#endif

#define DEADMAN_PIN 17

Button2 deadman(DEADMAN_PIN);

void deadman_init()
{
  deadman.setPressedHandler([](Button2 &btn) {
    // DEBUG("deadman pressed!");
    send_to_deadman_event_queue(EV_DEADMAN_PRESSED);
  });
  deadman.setReleasedHandler([](Button2 &btn) {
    // DEBUG("deadman released!");
    send_to_deadman_event_queue(EV_DEADMAN_RELEASED);
  });
}