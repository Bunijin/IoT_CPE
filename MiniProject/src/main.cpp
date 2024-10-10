#include <Arduino.h>

#include <Adafruit_BME280.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Adafruit_Sensor.h>

#include "time.h"
#include <GS_SDHelper.h>
#include <SPI.h>
#include <Wire.h>

#include <WiFiManager.h>
// #include <BlynkSimpleEsp32.h>
#include <ESP_Google_Sheet_Client.h>

#define LDR_PIN 17
#define OLED_SDA 22
#define OLED_SCL 23
#define BME_SDA 19
#define BME_SCL 21

TwoWire I2C_OLED = TwoWire(0);
TwoWire I2C_BME = TwoWire(1);

// GOOGLE SHEET
#define PROJECT_ID ""
#define CLIENT_EMAIL ""
const char PRIVATE_KEY[] PROGMEM = "";
const char spreadsheetId[] = "";

// BLYNK
// #define BLYNK_TEMPLATE_ID ""
// #define BLYNK_TEMPLATE_NAME ""
// #define BLYNK_AUTH_TOKEN ""
// char blynk_token[33] = BLYNK_AUTH_TOKEN;

// OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &I2C_OLED, -1);

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

// FUNCTION
void setupWiFi();
void setupOledDisplay();
void setupBME280();
void setupGoogleSheets();
void tokenStatusCallback(TokenInfo info);
unsigned long getTime();
String getFormattedTime();

// STATE
const int MEASURE = 1;
const int OLED_DISPLAY = 2;
const int BLYNK = 3;
const int SEND_SHEET = 4;
const int ALERT = 5;
int state;

// VARIABLE
float temperature, pressure, humidity, altitude;
int count, LDRReading;
String ldrState;
// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 30000;
// NTP server to request epoch time
const char *ntpServer = "pool.ntp.org";
// Variable to save current epoch time
unsigned long epochTime;

void setup()
{
  Serial.begin(115200);
  setupWiFi();
  setupOledDisplay();
  setupBME280();
  setupGoogleSheets();
  pinMode(LDR_PIN, INPUT);
  delayTime = 1000;
  state = MEASURE;
  count = 0;
}

void loop()
{
  bool ready = GSheet.ready();
  Serial.println(state);
  switch (state)
  {
  case MEASURE:
    delay(delayTime);
    // BME280
    temperature = bme.readTemperature();
    pressure = bme.readPressure() / 100.0F;
    altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);
    humidity = bme.readHumidity();
    // LDR
    ldrState = (digitalRead(LDR_PIN) == 1) ? "No Light" : "Light Detected";
    // YL-83
    // working on...
    state = OLED_DISPLAY;
    break;
  case OLED_DISPLAY:
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("T: ");
    display.print(temperature);
    display.println(" *C");

    display.print("P: ");
    display.print(pressure);
    display.println(" hPa");

    display.print("H: ");
    display.print(humidity);
    display.println(" %");

    display.print("LDR: ");
    display.print(ldrState);

    display.display();
    state = BLYNK;
    break;
  case BLYNK:
    // working on...
    state = SEND_SHEET;
    break;
  case SEND_SHEET:

    if (ready && millis() - lastTime > timerDelay)
    {
      lastTime = millis();

      FirebaseJson response;

      Serial.println("\nAppend spreadsheet values...");
      Serial.println("----------------------------");

      FirebaseJson valueRange;

      // Get timestamp
      epochTime = getTime();

      valueRange.add("majorDimension", "COLUMNS");
      valueRange.set("values/[0]/[0]", getFormattedTime());
      valueRange.set("values/[1]/[0]", String(temperature, 2));
      valueRange.set("values/[2]/[0]", String(pressure, 2));
      valueRange.set("values/[3]/[0]", String(altitude, 2));
      valueRange.set("values/[4]/[0]", String(humidity, 2));
      valueRange.set("values/[5]/[0]", ldrState);

      // For Google Sheet API ref doc, go to https://developers.google.com/sheets/api/reference/rest/v4/spreadsheets.values/append
      // Append values to the spreadsheet
      bool success = GSheet.values.append(&response /* returned response */, spreadsheetId /* spreadsheet Id to append */, "Sheet1!A1" /* range to append */, &valueRange /* data range to append */);
      if (success)
      {
        response.toString(Serial, true);
        valueRange.clear();
      }
      else
      {
        Serial.println(GSheet.errorReason());
      }
      Serial.println();
      Serial.println(ESP.getFreeHeap());
    }
    state = ALERT;
    break;
  case ALERT:
    // working on...
    state = MEASURE;
    break;
  default:
    state = MEASURE;
    break;
  }
}

void setupWiFi()
{
  // WiFi.mode(WIFI_STA);
  WiFiManager wm;
  // wm.resetSettings();
  bool res;
  // res = wm.autoConnect();                             // auto generated AP name from chipid
  res = wm.autoConnect("bunijinESP"); // anonymous ap
  // res = wm.autoConnect("AutoConnectAP", "password");  // password protected ap

  if (!res)
  {
    Serial.println("Failed to connect");
    // ESP.restart();
  }
  else
  {
    Serial.println("Connected!");
  }
}

void setupOledDisplay()
{
  I2C_OLED.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println(F("SSD1306 allocation failed"));
    while (true)
      ;
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("OLED Init Done");
  display.display();
}

void setupBME280()
{
  I2C_BME.begin(BME_SDA, BME_SCL);
  if (!bme.begin(0x76, &I2C_BME) && !bme.begin(0x77, &I2C_BME))
  {
    Serial.println(F("Could not find a valid BME280 sensor, check wiring!"));
    while (1)
      ;
  }
}

void setupGoogleSheets()
{
  configTime(0, 0, ntpServer);
  GSheet.printf("ESP Google Sheet Client v%s\n\n", ESP_GOOGLE_SHEET_CLIENT_VERSION);
  // Set the callback for Google API access token generation status (for debug only)
  GSheet.setTokenCallback(tokenStatusCallback);

  // Set the seconds to refresh the auth token before expire (60 to 3540, default is 300 seconds)
  GSheet.setPrerefreshSeconds(10 * 60);

  // Begin the access token generation for Google API authentication
  GSheet.begin(CLIENT_EMAIL, PROJECT_ID, PRIVATE_KEY);
}

unsigned long getTime()
{
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    // Serial.println("Failed to obtain time");
    return (0);
  }
  time(&now);
  return now;
}

String getFormattedTime()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    return "Failed to obtain time";
  }

  char timeStr[20];
  strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
  return String(timeStr);  // Return formatted string time
}


void tokenStatusCallback(TokenInfo info)
{
  if (info.status == token_status_error)
  {
    GSheet.printf("Token info: type = %s, status = %s\n", GSheet.getTokenType(info).c_str(), GSheet.getTokenStatus(info).c_str());
    GSheet.printf("Token error: %s\n", GSheet.getTokenError(info).c_str());
  }
  else
  {
    GSheet.printf("Token info: type = %s, status = %s\n", GSheet.getTokenType(info).c_str(), GSheet.getTokenStatus(info).c_str());
  }
}
