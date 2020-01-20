
#define LONGCLICK_MS 1000

#include <Button2.h>

#define BUTTON_0 0

Button2 button0(BUTTON_0);

void button_init()
{
  button0.setPressedHandler([](Button2 &btn) {
#ifdef FEATURE_CRUISE_CONTROL
    controller_packet.cruise_control = true;
#endif
  });
  button0.setReleasedHandler([](Button2 &btn) {
#ifdef FEATURE_CRUISE_CONTROL
    controller_packet.cruise_control = false;
#endif
  });
  button0.setLongClickHandler([](Button2 &btn) {
  });
}
