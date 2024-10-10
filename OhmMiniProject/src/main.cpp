#define BLYNK_TEMPLATE_ID ""
#define BLYNK_TEMPLATE_NAME ""
#define BLYNK_AUTH_TOKEN ""

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <WiFiManager.h>
#include <WiFiClientSecure.h>
#include <BlynkSimpleEsp8266.h>
#include <TimeLib.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);

#define DHTPIN 2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

#define LED_GREEN D6
#define LED_RED D5
#define BUZZER_PIN D7

const char *LINE_TOKEN = "";
const int GAS_THRESHOLD = 200;

WiFiUDP udp;
NTPClient timeClient(udp, "pool.ntp.org", 25200, 60000); // UTC+7
const char *host = "script.google.com";
const int httpsPort = 443;

String GAS_ID = ""; //--> spreadsheet script ID

enum State
{
  NORMAL,
  WARNING
};
State currentState;

float humidityValues[10];
float temperatureValues[10];
float gasValues[10];
int dataIndex = 0;

static unsigned long lastDisplayTime = 0;
static unsigned long lastLineNotifyTime = 0;
static bool showGas = true;
unsigned long currentMillis = millis();

float humidity, temperature, gasValue;
void sendLineNotify(const String &message);
void readSensors(float &humidity, float &temperature, float &gasValue);
void updateDisplayAndState(float gasValue, float temperature, float humidity, bool showGas);
String getTimeInThailand();
void sendData(float gasValue, float temperature, float humidity);

void setup()
{
  Serial.begin(115200);
  WiFiManager wm;
  //wm.resetSettings();
  wm.autoConnect("OHMESP", "password");
  Serial.println("Connected to WiFi");

  Blynk.begin(BLYNK_AUTH_TOKEN, WiFi.SSID().c_str(), WiFi.psk().c_str());
  lcd.init();
  lcd.backlight();
  dht.begin();

  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  timeClient.begin();  // Start NTP client
  timeClient.update(); // Update time

  currentState = NORMAL;
}

void loop()
{
  Blynk.run();
  unsigned long currentMillis = millis(); // Capture the current time once

  if (currentState == NORMAL)
  {
    if (currentMillis - lastDisplayTime >= 3000)
    {
      lastDisplayTime = currentMillis;
      showGas = !showGas;

      readSensors(humidity, temperature, gasValue);

      Blynk.virtualWrite(V2, gasValue);
      Blynk.virtualWrite(V0, temperature);
      Blynk.virtualWrite(V1, humidity);

      updateDisplayAndState(gasValue, temperature, humidity, showGas);
      sendData(gasValue, temperature, humidity); // Send data to Google Sheets

      // Transition to WARNING state if gasValue exceeds the threshold
      if (gasValue > GAS_THRESHOLD)
      {
        currentState = WARNING;
        return; // Exit the loop to enter WARNING state in next iteration
      }

      // Send LINE notification if it's time to do so
      if (currentMillis - lastLineNotifyTime >= 30000)
      { // Every 30 seconds
        lastLineNotifyTime = currentMillis;
        String currentTime = getTimeInThailand();
        sendLineNotify("Status: Normal  Gas PPM: " + String(gasValue) +
                       " Temp: " + String(temperature) + " Humidity: " +
                       String(humidity) + " Time: " + currentTime);
      }
    }
  }
  else if (currentState == WARNING)
  {
    // Handle WARNING state logic here
    if (currentMillis - lastLineNotifyTime >= 30000)
    { // Every 30 seconds
      lastLineNotifyTime = currentMillis;
      String currentTime = getTimeInThailand();
      sendLineNotify("badddd  Gas PPM: " + String(gasValue) +
                     " Temp: " + String(temperature) + " Humidity: " +
                     String(humidity) + " Time: " + currentTime);
    }
    currentState = NORMAL;
  }
}

void sendLineNotify(const String &message)
{
  WiFiClientSecure client;
  client.setInsecure();

  if (!client.connect("notify-api.line.me", 443))
  {
    Serial.println("Connection to LINE failed");
    return;
  }

  String payload = "message=" + message;
  client.print(String("POST /api/notify HTTP/1.1\r\n") +
               "Host: notify-api.line.me\r\n" +
               "Authorization: Bearer " + String(LINE_TOKEN) + "\r\n" +
               "Content-Type: application/x-www-form-urlencoded\r\n" +
               "Content-Length: " + String(payload.length()) + "\r\n" +
               "Connection: close\r\n\r\n" + payload);

  Serial.println("Message sent to LINE: " + message);
}

void readSensors(float &humidity, float &temperature, float &gasValue)
{
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature))
  {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  int sensorValue = analogRead(A0);
  float voltage = sensorValue * (5.0 / 1023.0);
  gasValue = voltage * 100;

  // Store values in arrays
  humidityValues[dataIndex] = humidity;
  temperatureValues[dataIndex] = temperature;
  gasValues[dataIndex] = gasValue;
  dataIndex = (dataIndex + 1) % 10; // Circular buffer
}

void updateDisplayAndState(float gasValue, float temperature, float humidity, bool showGas)
{
  lcd.clear();
  if (!showGas)
  {
    lcd.setCursor(0, 0);
    lcd.print("Temp: ");
    lcd.print(temperature);
    lcd.print(" C");

    lcd.setCursor(0, 1);
    lcd.print("Humidity: ");
    lcd.print(humidity);
    lcd.print(" %");
  }
  else
  {
    lcd.setCursor(0, 0);
    lcd.print("Gas PPM: ");
    lcd.print(gasValue);
    if (currentState == WARNING)
    {
      lcd.setCursor(0, 1);
      lcd.print("WARNING: DANGER!");
      digitalWrite(LED_RED, HIGH);
      digitalWrite(LED_GREEN, LOW);
      tone(BUZZER_PIN, 1000);
      sendLineNotify("WARNING: DANGER! Gas PPM: " + String(gasValue) +
                     " Temp: " + String(temperature) + " Humidity: " + String(humidity));
      Blynk.virtualWrite(V3, 1); // Turn on RED LED
      Blynk.virtualWrite(V4, 0); // Turn off GREEN LED
    }
    else if (currentState == NORMAL)
    {
      lcd.setCursor(0, 1);
      lcd.print("Status: Normal");
      digitalWrite(LED_GREEN, HIGH);
      digitalWrite(LED_RED, LOW);
      noTone(BUZZER_PIN);
      Blynk.virtualWrite(V3, 0); // Turn off RED LED
      Blynk.virtualWrite(V4, 1); // Turn on GREEN LED
    }
  }
}

String getTimeInThailand()
{
  timeClient.update();                    // Update time
  time_t now = timeClient.getEpochTime(); // Get Epoch time
  char buffer[80];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", localtime(&now)); // Use localtime to adjust time
  return String(buffer);
}

void sendData(float gasValue, float temperature, float humidity)
{
  Serial.println("==========");
  Serial.print("Connecting to ");
  Serial.println(host);

  WiFiClientSecure client;
  client.setInsecure();

  if (!client.connect(host, httpsPort))
  {
    Serial.println("Connection to Google Sheets failed");
    return;
  }

  String url = "/macros/s/" + GAS_ID + "/exec?temperature=" + String(temperature) + "&humidity=" + String(humidity) + "&gasValue=" + String(gasValue);
  Serial.print("Requesting URL: ");
  Serial.println(url);

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: BuildFailureDetectorESP8266\r\n" +
               "Connection: close\r\n\r\n");
  Serial.println("Request sent");
}