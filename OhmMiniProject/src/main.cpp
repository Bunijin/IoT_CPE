#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <Adafruit_Sensor.h>
// define sensor pin
#define DHTPIN D4
#define DHTTYPE DHT11 
#define MQ135 A0
// object lcd & dht
LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(DHTPIN, DHTTYPE);
// state
const int READ_DHT = 1;
const int READ_GAS = 2;
const int WRITE_LCD = 3;
// variable
int state, sensorValue;
bool isDHT = true;
float tempurature, humidity;

void setup() {
  Serial.begin(115200);
  // initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  // initialize DHT
  dht.begin();

  state = READ_DHT;
}

void loop() {
  switch (state)
  {
  case READ_DHT:
    // store TEMP and HUMID into variable
    tempurature = dht.readTemperature();
    humidity = dht.readHumidity();
    state = WRITE_LCD;
    break;
  case READ_GAS :
    // store GAS into variable
    sensorValue = analogRead(MQ135);
    state = WRITE_LCD;
    break;
  case WRITE_LCD:
    // clear screen before using
    lcd.clear();
    if(isDHT) {
      // display TEMP and HUMID on LCD
      lcd.setCursor(0,0);
      lcd.print("Temp  : ");
      lcd.print(tempurature);
      lcd.print(" C");
      lcd.setCursor(0,1);
      lcd.print("Humid : ");
      lcd.print(humidity);
      lcd.print(" %");
      // for telling next time to print GAS on LCD
      isDHT = false;
      state = READ_GAS;
    }
    else {
      // display GAS on LCD
      lcd.setCursor(0,0);
      lcd.print("Acetone ADC :");
      lcd.setCursor(0,1);
      lcd.print(sensorValue);
      // for telling next time to print DHT on LCD      
      isDHT = true;
      state = READ_DHT;
    }
    break;
  default:
    break;
  }
  delay(1000);
}