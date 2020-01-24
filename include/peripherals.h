
#define LONGCLICK_MS 1000

#include <Button2.h>

#define BUTTON_0 0

Button2 button0(BUTTON_0);

void button0_init()
{
  button0.setPressedHandler([](Button2 &btn) {
#ifdef FEATURE_CRUISE_CONTROL
    controller_packet.cruise_control = true;
#endif
    display_state.trigger(DISP_EV_BUTTON_CLICK);
  });
  button0.setReleasedHandler([](Button2 &btn) {
#ifdef FEATURE_CRUISE_CONTROL
    controller_packet.cruise_control = false;
#endif
  });
  button0.setLongClickHandler([](Button2 &btn) {
  });
}