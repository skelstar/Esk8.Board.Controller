

#ifndef Button2
#include <Button2.h>
#endif

#define DEADMAN_PIN 0

Button2 deadman(DEADMAN_PIN);

void deadman_init()
{
  deadman.setPressedHandler([](Button2 &btn) {
    DEBUG("deadman pressed!");
  });
  deadman.setReleasedHandler([](Button2 &btn) {
    DEBUG("deadman released!");
  });
}