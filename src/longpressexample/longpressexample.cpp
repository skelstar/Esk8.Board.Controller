/////////////////////////////////////////////////////////////////

#include "Button2.h"

/////////////////////////////////////////////////////////////////

#define BUTTON_PIN 21

/////////////////////////////////////////////////////////////////

Button2 button = Button2(BUTTON_PIN);

/////////////////////////////////////////////////////////////////

void longpress(Button2 &btn);
void longpress_detected(Button2 &btn);

/////////////////////////////////////////////////////////////////

void setup()
{
  Serial.begin(9600);
  while (!Serial)
  {
    delay(20);
  }
  Serial.println("\n\nLongpress Handler Demo");

  button.setLongClickHandler(longpress);
  button.setLongClickDetectedHandler(longpress_detected);
  button.setLongClickDetectedRetriggerable(true);
}

/////////////////////////////////////////////////////////////////

void loop()
{
  button.loop();
}

/////////////////////////////////////////////////////////////////

void longpress(Button2 &btn)
{
  unsigned int time = btn.wasPressedFor();
  Serial.print("You clicked ");
  if (time > 1500)
  {
    Serial.print("a really really long time.");
  }
  else if (time > 1000)
  {
    Serial.print("a really long time.");
  }
  else if (time > 500)
  {
    Serial.print("a long time.");
  }
  else
  {
    Serial.print("long.");
  }
  Serial.print(" (");
  Serial.print(time);
  Serial.println(" ms)");
}

void longpress_detected(Button2 &btn)
{
  Serial.print("Longpress detected!\n");
}

/////////////////////////////////////////////////////////////////