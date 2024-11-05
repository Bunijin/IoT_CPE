#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// reset & slave select
#define RST_PIN D3
#define SS_PIN D8
#define LED_PIN D1
// MQTT Broker settings
const char *mqtt_broker = "broker.emqx.io";
const char *mqtt_topic = "BananaChicken/pub";
const char *mqtt_topic_sub = "BananaChicken/sub";
const int mqtt_port = 1883;
// objects
MFRC522 mfrc522(SS_PIN, RST_PIN);
WiFiClient espClient;
PubSubClient mqtt_client(espClient);
JsonDocument doc_pub, doc_sub;
// variables
String mqttMessage;
String msg;
String rfid_in = "";
char jsonBuffer[512];
// functions
String dump_byte_array(byte *buffer, byte bufferSize);
void setupWifi();
void connectToMQTTBroker();
void mqttCallback(char *topic, byte *payload, unsigned int length);

enum State
{
  IDLE,
  CARD_TOUCH,
  OPEN_DOOR
};
State state;

void setup()
{
  state = IDLE;
  Serial.begin(115200);
  setupWifi();
  SPI.begin();
  mfrc522.PCD_Init();
  mqtt_client.setServer(mqtt_broker, mqtt_port);
  mqtt_client.setCallback(mqttCallback);
  connectToMQTTBroker();
  pinMode(LED_PIN, OUTPUT);
  mfrc522.PCD_DumpVersionToSerial();
}

void loop()
{
  if (state == IDLE)
  {
    mqtt_client.loop();
    connectToMQTTBroker();
    if (mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial())
    {
      state = CARD_TOUCH;
    }
    if (msg == "Opening door.")
    {
      state = OPEN_DOOR;
    }
    delay(1000);
  }
  else if (state == CARD_TOUCH)
  {
    // read RFID id
    rfid_in = dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
    doc_pub["rfid_id"] = rfid_in;
    serializeJson(doc_pub, jsonBuffer);
    mqtt_client.publish(mqtt_topic, jsonBuffer);
    state = IDLE;
  }
  else if (state == OPEN_DOOR)
  {
    digitalWrite(LED_PIN, HIGH);
    delay(3000);
    digitalWrite(LED_PIN, LOW);
    msg = "";
    state = IDLE;
  }
}

String dump_byte_array(byte *buffer, byte bufferSize)
{
  String content = "";
  for (byte i = 0; i < bufferSize; i++)
  {
    content.concat(String(buffer[i] < 0x10 ? " 0" : " "));
    content.concat(String(buffer[i], HEX));
  }
  content.toUpperCase();
  return content;
}

void setupWifi()
{
  WiFiManager wm;
  bool res;
  res = wm.autoConnect("BNNCHKN");
  if (!res)
  {
    Serial.println("[ERROR] Failed to connect");
    wm.resetSettings();
    ESP.restart();
  }
  else
  {
    Serial.println("Connected successfully");
  }
}

void connectToMQTTBroker()
{
  while (!mqtt_client.connected())
  {
    String client_id = "esp8266-client-" + String(WiFi.macAddress());
    Serial.printf("Connecting to MQTT Broker as %s.....\n", client_id.c_str());
    if (mqtt_client.connect(client_id.c_str()))
    {
      Serial.println("Connected to MQTT broker");
      mqtt_client.subscribe(mqtt_topic_sub);
    }
    else
    {
      Serial.print("Failed to connect to MQTT broker, rc=");
      Serial.print(mqtt_client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void mqttCallback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message received on topic: ");
  Serial.println(topic);
  Serial.print("Message: ");
  mqttMessage = "";
  for (unsigned int i = 0; i < length; i++)
  {
    mqttMessage += (char)payload[i];
  }
  deserializeJson(doc_sub, mqttMessage);
  const char *doc_sub_message = doc_sub["message"];
  Serial.println(doc_sub_message);
  msg = doc_sub_message;
}
