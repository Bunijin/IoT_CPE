#include <Arduino.h>

const int motor = 0;
int state;
int value;

void setup()
{
  Serial.begin(115200);
  pinMode(D1, OUTPUT);
  pinMode(D2, OUTPUT);
  state = motor;
  value = 0;
}

void loop()
{
  switch (state)
  {
  case motor:
    value = analogRead(A0);
    analogWrite(D1, 0);
    analogWrite(D2, value);
    Serial.println(value);
    state = motor;
    break;
  }
}