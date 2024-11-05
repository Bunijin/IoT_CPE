#include <Arduino.h>
#include <Adafruit_BME280.h>  // sensor
#include <Adafruit_SSD1306.h> // display
#include <Adafruit_GFX.h>     // for display
#include "time.h"             // for sheets
#include <Wire.h>
#include <WiFi.h>
#include <ESP_Google_Sheet_Client.h>
#include <HTTPClient.h>
TwoWire I2C_OLED = TwoWire(0);
TwoWire I2C_BME = TwoWire(1);
// GOOGLE SHEET
#define PROJECT_ID ""
#define CLIENT_EMAIL ""
#define PRIVATE_KEY ""
const char spreadsheetId[] = "";
// BLYNK
#define WIFI_SSID ""
#define WIFI_PASSWORD ""
#define BLYNK_TEMPLATE_ID ""
#define BLYNK_TEMPLATE_NAME ""
#define BLYNK_AUTH_TOKEN ""
#include <BlynkSimpleEsp32.h>
// OLED
#define OLED_SDA 22
#define OLED_SCL 23
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &I2C_OLED, -1);

// BME280
#define BME_SDA 19
#define BME_SCL 21
#define BME_SCK 13
#define BME_MISO 12
#define BME_MOSI 11
#define BME_CS 10
#define SEALEVELPRESSURE_HPA (1013.25)
Adafruit_BME280 bme;

#define LDR_PIN 33
#define WATER_PIN 34
#define LINE_TOKEN ""
// FUNCTION
void setupWifi();
void reconnectWifi();
void setupOledDisplay();
void setupBME280();
void setupGoogleSheets();
void tokenStatusCallback(TokenInfo info);
unsigned long getTime();
const char *getFormattedTime();
void sendLineNotify(String message);
float mapFloat(long x, long in_min, long in_max, long out_min, long out_max);
// STATE
enum State
{
  MEASURE,
  OLED_DISPLAY,
  BLYNK,
  SEND_SHEET,
  ALERT
};
State state;
// VARIABLE
float temperature, pressure, humidity, LDRReading, WaterReading;
bool showFirstScreen = true;
String msg;
// Timer variables
unsigned long delayTime = 5 * 1000;
unsigned long sheetsLastTime = 0;
unsigned long lineLastTime = 0;
unsigned long sheetsTimerDelay = 1 * 60 * 1000;
unsigned long lineTimerDelay = 0.5 * 60 * 1000;
// NTP server to request epoch time
const char *ntpServer = "pool.ntp.org";
// Variable to save current epoch time
unsigned long epochTime;
const char *blynk_token = BLYNK_AUTH_TOKEN;

void setup()
{
  Serial.begin(115200);
  setupWifi();
  setupOledDisplay();
  setupBME280();
  setupGoogleSheets();
  pinMode(LDR_PIN, INPUT);
  pinMode(WATER_PIN, INPUT);
  Blynk.begin(blynk_token, WIFI_SSID, WIFI_PASSWORD);
  state = MEASURE;
}

void loop()
{
  bool ready = GSheet.ready();
  Blynk.run();
  switch (state)
  {
  case MEASURE:
    delay(delayTime);
    if (WiFi.status() != WL_CONNECTED)
      reconnectWifi();
    if (temperature < -40 || temperature > 85 || isnan(temperature) || isnan(pressure) || isnan(humidity))
      setupBME280();
    temperature = bme.readTemperature();
    pressure = bme.readPressure() / 100.0F;
    humidity = bme.readHumidity();
    LDRReading = mapFloat(analogRead(LDR_PIN), 4095, 0, 0, 4095.0);
    WaterReading = mapFloat(analogRead(WATER_PIN), 4095, 0, 0, 10.0);
    state = OLED_DISPLAY;
    break;
  case OLED_DISPLAY:
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(0, 0);
    if (showFirstScreen)
    {
      display.print("T:");
      display.print(temperature);
      display.println("*C");
      display.print("H:");
      display.print(humidity);
      display.println("%");
      display.println("P: ");
      display.print(pressure);
      display.println("hPa");
      showFirstScreen = false;
    }
    else
    {
      display.println("Light: ");
      display.println((int)LDRReading);
      display.println("Water: ");

      if (WaterReading > 5.4 && WaterReading <= 8.8)
        display.print("Light ");
      else if (WaterReading <= 5.4)
        display.print("No ");
      display.println("Rain");
      display.display();
      showFirstScreen = true;
    }
    display.display();
    state = BLYNK;
    break;
  case BLYNK:
    Blynk.virtualWrite(V0, temperature);
    Blynk.virtualWrite(V1, humidity);
    Blynk.virtualWrite(V2, pressure);
    Blynk.virtualWrite(V3, LDRReading);
    Blynk.virtualWrite(V4, String((WaterReading > 5.4 && WaterReading <= 8.8 ? "Light " : (WaterReading <= 5.4 ? "No " : ""))) + "Rain");
    state = SEND_SHEET;
    break;
  case SEND_SHEET:
    if (ready && millis() - sheetsLastTime > sheetsTimerDelay)
    {
      sheetsLastTime = millis();
      FirebaseJson response;
      Serial.println("\nAppend spreadsheet values...");
      FirebaseJson valueRange;
      // Get timestamp
      epochTime = getTime();
      String waterLevel = String((WaterReading > 5.4 && WaterReading <= 8.8 ? "Light " : (WaterReading <= 5.4 ? "No " : ""))) + "Rain";
      valueRange.add("majorDimension", "COLUMNS");
      valueRange.set("values/[0]/[0]", getFormattedTime());
      valueRange.set("values/[1]/[0]", String(temperature, 2));
      valueRange.set("values/[2]/[0]", String(pressure, 2));
      valueRange.set("values/[3]/[0]", String(humidity, 2));
      valueRange.set("values/[4]/[0]", LDRReading);
      valueRange.set("values/[5]/[0]", waterLevel);

      // For Google Sheet API ref doc, go to https://developers.google.com/sheets/api/reference/rest/v4/spreadsheets.values/append
      // Append values to the spreadsheet
      bool success = GSheet.values.append(&response /* returned response */, spreadsheetId /* spreadsheet Id to append */, "Sheet1!A2" /* range to append */, &valueRange /* data range to append */);
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
    if (ready && millis() - lineLastTime > lineTimerDelay)
    {
      lineLastTime = millis();
      // Temperature alerts
      msg += "Data\r\n";
      if (temperature > 34)
        msg += "High ";
      else if (temperature <= 25)
        msg += "Low ";
      msg += "Temp: ";
      msg += temperature;
      msg += " %2AC \r\n";
      // Humidity alerts
      if (humidity > 90)
        msg += "High ";
      else if (humidity <= 50)
        msg += "Low ";
      msg += "Humidity: ";
      msg += humidity;
      msg += " %25 \r\n";
      // Pressure alerts
      if (pressure <= 990)
        msg += "Very Low ";
      else if (pressure > 990 && pressure <= 1000)
        msg += "Low ";
      else if (pressure > 1020)
        msg += "High ";
      msg += "Pressure: ";
      msg += pressure;
      msg += " hPa \r\n";
      // Light conditions
      msg += "Light: ";
      msg += (int)LDRReading;
      msg += "\r\n";
      // Rain detection
      if (WaterReading > 5.4 && WaterReading <= 8.8)
        msg += "Light ";
      else if (WaterReading <= 5.4)
        msg += "No ";
      msg += "Rain";
      // Trim any trailing space or semicolon
      msg.trim();
      Serial.println("");
      if (msg.length() > 0)
      {
        sendLineNotify(msg);
      }
    }
    msg = "";

    state = MEASURE;
    break;
  default:
    state = MEASURE;
    break;
  }
}

void setupWifi()
{
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print(".");
  }
  Serial.println();
}

void reconnectWifi()
{
  Serial.println("Wi-Fi lost. Reconnecting...");
  WiFi.disconnect();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println(WiFi.status() == WL_CONNECTED ? "\nReconnected to Wi-Fi" : "\nFailed to reconnect to Wi-Fi");
}

void setupOledDisplay()
{
  I2C_OLED.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println(F("SSD1306 allocation failed"));
    return;
  }
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("OLED Init Done");
  display.display();
}

void setupBME280()
{
  bool status = I2C_BME.begin(BME_SDA, BME_SCL);
  if (!status)
    Serial.println("\nI2C initialization failed, check wiring!");
  if (!bme.begin(0x76, &I2C_BME) && !bme.begin(0x77, &I2C_BME))
    Serial.println("\nCould not find a valid BME280 sensor, check wiring!");
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

const char *getFormattedTime()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    return "Failed to obtain time"; // Return a constant string
  }
  timeinfo.tm_hour += 7;
  if (timeinfo.tm_hour >= 24)
  {
    timeinfo.tm_hour -= 24;
    timeinfo.tm_mday += 1;
  }
  static char timeStr[20]; // Make this static to persist after the function returns
  strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
  return timeStr; // Return the formatted string time
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

void sendLineNotify(String message)
{

  if (WiFi.status() == WL_CONNECTED)
  { // Check if connected to WiFi
    HTTPClient http;
    http.begin("https://notify-api.line.me/api/notify"); // LINE Notify API URL
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    http.addHeader("Authorization", "Bearer " + String(LINE_TOKEN));
    // Send message
    int httpResponseCode = http.POST("message=" + message);
    if (httpResponseCode > 0)
    {
      Serial.print("HTTP Response code: ");
      Serial.println(httpResponseCode);
    }
    else
    {
      Serial.print("Error code: ");
      Serial.println(httpResponseCode);
    }
    http.end(); // Free resources
  }
  else
  {
    Serial.println("WiFi Disconnected");
  }
}

float mapFloat(long x, long in_min, long in_max, long out_min, long out_max)
{
  return (float)(x - in_min) * (out_max - out_min) / (float)(in_max - in_min) + out_min;
}