
// #define LONGCLICK_MS 1000

#include <Button2.h>

#define END_LIGHT_PIN 32

Adafruit_NeoPixel endLights(/*num*/ 5, END_LIGHT_PIN, NEO_GRB + NEO_KHZ800);

#define BUTTON_0 0
Button2 button0(BUTTON_0);

#define DEADMAN_PIN 27
Button2 deadman(DEADMAN_PIN);

class EndLight
{
public:
  void init(Adafruit_NeoPixel *lights)
  {
    _lights = lights;
    _lights->begin();
    _lights->show();
    _lights->setBrightness(50);

    _state = OFF;
  }

  void toggle()
  {
    switch (_state)
    {
    case OFF:
      _state = ON;
      _set(COLOUR_WHITE);
      break;
    case ON:
      _state = OFF;
      _set(COLOUR_OFF);
      break;
    }
  }

  void loop()
  {
    uint8_t e;
    if (xEndLightEventQueue != NULL && xQueueReceive(xEndLightEventQueue, &e, (TickType_t)1) == pdPASS)
    {
      toggle();
    }
  }

private:
  enum LightState
  {
    OFF,
    ON
  };

  Adafruit_NeoPixel *_lights;
  LightState _state;

  uint32_t COLOUR_WHITE = _lights->Color(255, 255, 255);
  uint32_t COLOUR_OFF = _lights->Color(0, 0, 0);

  void _set(uint32_t colour)
  {
    for (int i = 0; i < _lights->numPixels(); i++)
    {
      _lights->setPixelColor(i, colour);
    }
    _lights->show();
  }
};

EndLight endLight;

//---------------------------------------------------------
void button0_init()
{
  button0.setClickHandler([](Button2 &btn) {
    display_state.trigger(DISP_EV_MENU_BUTTON_CLICKED);
  });
  button0.setDoubleClickHandler([](Button2 &btn) {
    if (display_task_showing_option_screen)
    {
      send_to_display_event_queue(DISP_EV_MENU_OPTION_SELECT);
    }
    else
    {
    }
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
