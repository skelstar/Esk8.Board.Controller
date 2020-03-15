
// #define LONGCLICK_MS 1000

#include <Button2.h>

#define BUTTON_0 0
Button2 button0(BUTTON_0);

#define DEADMAN_PIN 27
Button2 deadman(DEADMAN_PIN);

void button0_init()
{
  button0.setClickHandler([](Button2 &btn) {
#ifdef FEATURE_CRUISE_CONTROL
    controller_packet.cruise_control = true;
#endif
    // display_state.trigger(DISP_EV_BUTTON_CLICK);
  });
  button0.setReleasedHandler([](Button2 &btn) {
#ifdef FEATURE_CRUISE_CONTROL
    controller_packet.cruise_control = false;
#endif
  });
  button0.setDoubleClickHandler([](Button2 &btn) {
    // display_state.trigger(DISP_EV_MENU_OPTION_SELECT);
  });
}

void button35_init()
{
  button35.setClickHandler([](Button2 &btn) {
  });
  button35.setReleasedHandler([](Button2 &btn) {
  });
  button35.setDoubleClickHandler([](Button2 &btn) {
  });
}

void deadman_init()
{
}
