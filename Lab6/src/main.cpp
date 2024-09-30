#include <Arduino.h>
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <PubSubClient.h>

// MQTT Broker settings
const char *mqtt_broker = "broker.emqx.io"; // EMQX broker endpoint
const char *mqtt_topic = "BananaChicken";   // MQTT topic
// const char *mqtt_username = "username";          // MQTT username for authentication
// const char *mqtt_password = "password";        // MQTT password for authentication
const int mqtt_port = 1883; // MQTT port (TCP)

WiFiClient espClient;
PubSubClient mqtt_client(espClient);
String mqttMessage;

const int IDLE = 1;
const int BUTTON_LED = 2;
const int MQTT_LED = 3;
int state;
int flag = false;
String msg = "";
int button_led = 0;

IRAM_ATTR void handleInterrupt();

void setupWiFiManager();
void connectToMQTTBroker();
void mqttCallback(char *topic, byte *payload, unsigned int length);

void setup()
{
  Serial.begin(115200);
  setupWiFiManager();

  mqtt_client.setServer(mqtt_broker, mqtt_port);
  mqtt_client.setCallback(mqttCallback);
  connectToMQTTBroker();

  pinMode(D7, OUTPUT);
  pinMode(D6, INPUT);
  attachInterrupt(digitalPinToInterrupt(D6), handleInterrupt, CHANGE);
  state = IDLE;
}

void loop()
{
  if (state == IDLE)
  {
    msg = "";
    mqtt_client.loop();
    connectToMQTTBroker();
    msg = mqttMessage;
    if (msg == "on" || msg == "off") {
      state = MQTT_LED;
    }
    if (flag == true)
    {
      flag = false;
      state = BUTTON_LED;
    }
    delay(1000);
  }
  else if (state == MQTT_LED)
  {
    if (msg == "on") {
      digitalWrite(D7, HIGH);
    } else if (msg == "off") {
      digitalWrite(D7, LOW);
    }
    mqttMessage = "";
    state = IDLE;
  }
  else if (state == BUTTON_LED)
  {
    if (digitalRead(D7) == LOW)
    {
      digitalWrite(D7, HIGH);
      mqtt_client.publish(mqtt_topic, "LED ON");
    }
    else if (digitalRead(D7) == HIGH)
    {
      digitalWrite(D7, LOW);
      mqtt_client.publish(mqtt_topic, "LED OFF");
    }
    msg = "";
    state = IDLE;
  }
}

void setupWiFiManager()
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
  res = wm.autoConnect("BananaChickenAP"); // anonymous ap
  // res = wm.autoConnect("AutoConnectAP","password"); // password protected ap

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

void connectToMQTTBroker()
{
  while (!mqtt_client.connected())
  {
    String client_id = "esp8266-client-" + String(WiFi.macAddress());
    Serial.printf("Connecting to MQTT Broker as %s.....\n", client_id.c_str());
    // if (mqtt_client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
    if (mqtt_client.connect(client_id.c_str()))
    {
      Serial.println("Connected to MQTT broker");
      mqtt_client.subscribe(mqtt_topic);
      // Publish message upon successful connection
      mqtt_client.publish(mqtt_topic, "Hi EMQX I'm ESP8266 ^^");
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
    mqttMessage += (char)payload[i]; // Convert *byte to string
  }
  Serial.println(mqttMessage);
}

IRAM_ATTR void handleInterrupt()
{
  flag = true;
}