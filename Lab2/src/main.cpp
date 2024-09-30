#include <Arduino.h>

IRAM_ATTR void handleInterrupt_1();
IRAM_ATTR void handleInterrupt_2();
const int IDLE = 0;
const int PRESS_UP = 1;
const int PRESS_DOWN = 2;
int flag_up = false;
int flag_down = false;
int count = -1;
int state;
void setup()
{
  Serial.begin(115200);
  state = IDLE;
  pinMode(D4, OUTPUT);
  pinMode(D5, INPUT);
  pinMode(D6, INPUT);
  attachInterrupt(digitalPinToInterrupt(D5), handleInterrupt_2, RISING);
  attachInterrupt(digitalPinToInterrupt(D6), handleInterrupt_1, RISING);
}
void loop()
{
  if (state == IDLE)
  {
    digitalWrite(D4, HIGH);
    if (flag_up == true)
    {
      flag_up = false;
      delay(50);
      if (digitalRead(D6) == 1)
      {
        state = PRESS_UP;
      }
    }
    else if (flag_down == true)
    {
      flag_down = false;
      delay(50);
      if (digitalRead(D5) == 1)
      {
        state = PRESS_DOWN;
      }
    }
  }
  else if (state == PRESS_UP)
  {
    count++;
    if (count > 9)
    {
      count = 0;
    }
    Serial.println(count);
    state = IDLE;
  }
  else if (state == PRESS_DOWN)
  {
    count--;
    if (count < 0)
    {
      count = 9;
    }
    Serial.println(count);
    state = IDLE;
  }
}
IRAM_ATTR void handleInterrupt_1()
{
  flag_up = true;
}

IRAM_ATTR void handleInterrupt_2()
{
  flag_down = true;
}