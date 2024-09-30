#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

const int UPDATE_POS = 1;
const int WRITE_LCD = 2;

const char message[] = "RMUTT";
int pos[5] = {0, 1, 2, 3, 4};
int row[5] = {0, 0, 0, 0, 0};

int state;

void setup()
{
  lcd.init();
  lcd.backlight();
  lcd.clear();
  state = UPDATE_POS;
}

void loop()
{
  switch (state)
  {
  case UPDATE_POS:
  
    for (int num = 0; num < 5; num++)
    {
      pos[num]++;
      if (pos[num] >= 16)
      {
        pos[num] = 0;
        row[num] = (row[num] + 1) % 2;
      }
    }
    state = WRITE_LCD;
    break;

  case WRITE_LCD:

    lcd.clear();
    for (int num = 0; num < 5; num++)
    {
      lcd.setCursor(pos[num], row[num]);
      lcd.print(message[num]);
    }
    delay(500);
    state = UPDATE_POS;
    break;
  }
}
