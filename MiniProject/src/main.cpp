#include <Arduino.h>
#include <WiFiManager.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

// OLED
#define OLED_RESET 4

Adafruit_SSD1306 display(OLED_RESET);

// BME280
#define BME_SCK 13
#define BME_MISO 12
#define BME_MOSI 11
#define BME_CS 10

#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME280 bme; // I2C
// Adafruit_BME280 bme(BME_CS); // hardware SPI
// Adafruit_BME280 bme(BME_CS, BME_MOSI, BME_MISO, BME_SCK); // software SPI

unsigned long delayTime;

// LDR
int LDR_Pin = 34;

// FUNCTION
void setupWiFi();
void setupOledDisplay();
void setupBME280(int address);
void readBME();
void readLDR();

void setup()
{
  Serial.begin(115200);
  // setupWiFi();
  // setupOledDisplay();
  // setupBME280(0x76);
  // pinMode(LDR_Pin, INPUT);
}

void loop()
{

}

void setupWiFi()
{
  // WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP
  // it is a good practice to make sure your code sets wifi mode how you want it.

  // WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wm;

  // reset settings - wipe stored credentials for testing
  // these are stored by the esp library
  // wm.resetSettings();

  // Automatically connect using saved credentials,
  // if connection fails, it starts an access point with the specified name ( "AutoConnectAP"),
  // if empty will auto generate SSID, if password is blank it will be anonymous AP (wm.autoConnect())
  // then goes into a blocking loop awaiting configuration and will return success result

  bool res;
  // res = wm.autoConnect(); // auto generated AP name from chipid
  res = wm.autoConnect("AutoConnectAP"); // anonymous ap
  // res = wm.autoConnect("AutoConnectAP", "password"); // password protected ap

  if (!res)
  {
    Serial.println("Failed to connect");
    // ESP.restart();
  }
  else
  {
    // if you get here you have connected to the WiFi
    Serial.println("connected...yeey :)");
  }
}

void setupOledDisplay()
{
  display.begin(SSD1306_SWITCHCAPVCC, 0x3c); // initialize I2C addr 0x3c
  display.clearDisplay();                    // clears the screen and buffer
  display.drawPixel(127, 63, WHITE);

  display.drawLine(0, 63, 127, 21, WHITE);
  display.drawCircle(110, 50, 12, WHITE);
  display.fillCircle(45, 50, 8, WHITE);
  display.drawTriangle(70, 60, 90, 60, 80, 46, WHITE);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Welcome to");
  display.setTextSize(2);
  display.println("Bunijin");
  display.setTextColor(BLACK, WHITE);
  display.setTextSize(1);
  display.println("doki.ddns.me");
  display.setTextColor(WHITE, BLACK);
  display.display();
}
void setupBME280(int address)
{
  bool status;

  // default settings
  // (you can also pass in a Wire library object like &Wire2)
  status = bme.begin(address);
  if (!status)
  {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1)
      ;
  }
  delayTime = 1000;
}
void readBME()
{
  Serial.print("Temperature = ");
  Serial.print(bme.readTemperature()); // อ่านค่าอุณหภูมิ
  Serial.println(" *C");

  // Convert temperature to Fahrenheit
  /*Serial.print("Temperature = ");
   Serial.print(1.8 * bme.readTemperature() + 32);
   Serial.println(" *F");*/

  Serial.print("Pressure = ");
  Serial.print(bme.readPressure() / 100.0F); // อ่านค่าความกดอากาศ
  Serial.println(" hPa");

  Serial.print("Approx. Altitude = ");
  Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA)); // อ่านค่าประมาณของ Altitude หน่วยเป็นเมต อ้างอิงจากระดับความสูงจากทะเล
  Serial.println(" m");

  Serial.print("Humidity = ");
  Serial.print(bme.readHumidity()); // อ่านความชื้น
  Serial.println(" %");

  Serial.println();
}

void readLDR() {
  int LDRReading = digitalRead(LDR_Pin);

  if (LDRReading == HIGH)
  {
    Serial.println("Low light detected");
  }
  else
  {
    Serial.println("Bright light detected");
  }
}