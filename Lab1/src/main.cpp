#include <Arduino.h>

#define _led1 D2
#define _led2 D3
#define _led3 D6
#define _led4 D7

String state;
int blink_count = 0;

void setup()
{
  Serial.begin(115200);
  pinMode(_led1, OUTPUT);
  pinMode(_led2, OUTPUT);
  pinMode(_led3, OUTPUT);
  pinMode(_led4, OUTPUT);
  state = "led-1";
}

void loop()
{
  if (state == "led-1")
  {
    digitalWrite(_led4, LOW);
    digitalWrite(_led1, HIGH);
    state = "led-2";
    delay(250);
  }
  else if (state == "led-2")
  {
    digitalWrite(_led1, LOW);
    digitalWrite(_led2, HIGH);
    state = "led-3";
    delay(250);
  }
  else if (state == "led-3")
  {
    digitalWrite(_led2, LOW);
    digitalWrite(_led3, HIGH);
    state = "led-4";
    delay(250);
  }
  else if (state == "led-4")
  {
    digitalWrite(_led3, LOW);
    digitalWrite(_led4, HIGH);
    state = "led-1";
    delay(250);
  }
}