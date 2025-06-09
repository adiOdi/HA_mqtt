#include <WiFi.h>
#include "LittleFS.h"
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "sensor.h"
#include <MQTT.h>
#include <HTTPClient.h>
#include "SECRETS.h"

// Replace with your network credentials
const char *ssid = WIFI_SSID;
const char *password = WIFI_PWD;

// Create an instance of the server
AsyncWebServer server(80);
JsonDocument preset_json;
MQTTClient client;
WiFiClient net;

unsigned long lastMillis = 0;
#define RX_PIN 16
#define TX_PIN 17
#define BAUD_RATE 256000
#define MAX_TARGETS 3
#define MAX_SPACES 7
rd_03d radar;
bool occupation[MAX_SPACES] = {true, true, true, true, true, true, true};
bool isMoving = false;
String names[MAX_SPACES] = {"Radar Room", "Radar PC", "Radar not PC", "Radar Entry", "Radar Bed", "Radar Sofa", "Radar Window"};
void updateSettings(AsyncWebServerRequest *request, uint8_t *data)
{
  Serial.printf("%s\n", (const char *)data);
  DeserializationError error = deserializeJson(preset_json, data);
  if (error)
  {
    Serial.println(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    AsyncWebServerResponse *response = request->beginResponse(400, "plain/text", error.f_str());
    request->send(response);
    return;
  }
  radar.setSensor(preset_json["sensor"]["x"], preset_json["sensor"]["y"], preset_json["sensor"]["rot"]);
  File preset = LittleFS.open("/preset.json", "w");
  if (serializeJson(preset_json, preset))
  {
    Serial.println(F("File written successfully"));
  }
  else
  {
    Serial.println(F("Failed to write file"));
  }
  preset.close();
  File presetr = LittleFS.open("/preset.json");
  Serial.println(presetr.readString());
  presetr.close();
  AsyncWebServerResponse *response = request->beginResponse(200);
  request->send(response);
}
void connect()
{
  Serial.print("\nconnecting mqtt...");
  while (!client.connect("espRadar"))
  {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("\nconnected!");
  // client.subscribe("/hello");
  // client.unsubscribe("/hello");
}
void senddiscovery()
{
  for (size_t i = 0; i < MAX_SPACES; i++)
  {
    String topic = "homeassistant/binary_sensor/radar_zone_" + String(i + 1) + "/config";
    String payload = "{\"name\": \"" + names[i] + "\", \"state_topic\": \"espRadar/state\", \"unique_id\": \"radar_zone_" + String(i + 1) + "\", \"payload_on\": \"ON\",\"value_template\": \"{{ value_json.z" + String(i + 1) + " }}\"}";
    Serial.println("Publishing to " + topic + ": " + payload);
    client.publish(topic, payload, true, 0);
  }
  String topic = "homeassistant/binary_sensor/radar_speed_high/config";
  String payload = "{\"name\": \"Radar speed is high\", \"state_topic\": \"espRadar/state\", \"unique_id\": \"radar_speed_high\", \"payload_on\": \"ON\",\"value_template\": \"{{ value_json.s }}\"}";
  Serial.println("Publishing to " + topic + ": " + payload);
  client.publish(topic, payload, true, 0);
}
// void messageReceived(String &topic, String &payload)
// {
//   Serial.println("incoming: " + topic + " - " + payload);

//   // Note: Do not use the client in the callback to publish, subscribe or
//   // unsubscribe as it may cause deadlocks when other things arrive while
//   // sending and receiving acknowledgments. Instead, change a global variable,
//   // or push to a queue and handle it in the loop after calling `client.loop()`.
// }
void setup()
{
  Serial.begin(BAUD_RATE);

  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  // Print ESP Local IP Address
  Serial.println(WiFi.localIP());

  if (!LittleFS.begin())
  {
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }
  radar.setup(RX_PIN, TX_PIN);
  File preset_r = LittleFS.open("/preset.json");
  DeserializationError error = deserializeJson(preset_json, preset_r);
  if (error)
  {
    Serial.println(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }
  radar.setSensor(preset_json["sensor"]["x"], preset_json["sensor"]["y"], preset_json["sensor"]["rot"]);
  preset_r.close();

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/index.html", "text/html"); });
  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/script.js", "text/javascript"); });
  server.on("/Board.svg", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/Board.svg", "image/svg+xml"); });
  server.on("/preset.json", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/preset.json", "application/json"); });
  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "application/json",
                            radar.getTargetDataString(radar.getData()->target_1)); });
  // server.on("/update", HTTP_POST, updateSettings);
  server.onRequestBody([](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
                       {
    if (request->url() == "/update") {
      updateSettings(request, data);
    } });
  // Start server
  server.begin();
  IPAddress ip(MQTT_BROKER_IP_1, MQTT_BROKER_IP_2, MQTT_BROKER_IP_3, MQTT_BROKER_IP_4);
  client.begin(ip, MQTT_PORT, net);
  // client.onMessage(messageReceived);

  connect();
  senddiscovery();
}
String createMqttState()
{
  String state = "{";
  for (size_t i = 0; i < MAX_SPACES; i++)
  {
    state += "\"z" + String(i + 1) + "\": \"" + (occupation[i] ? "ON" : "OFF") + "\"";
    // if (i < MAX_SPACES - 1)
    state += ",";
  }
  state += "\"s\": \"" + String(radar.isMoving() ? "ON" : "OFF") + "\"";
  state += "}";
  return state;
}
void loop()
{
  client.loop();
  radar.update();
  bool changed = false;
  for (size_t i = 0; i < MAX_SPACES; i++)
  {
    bool occupied = radar.isOccupied(preset_json["spaces"][i]);
    if (occupied != occupation[i])
    {
      changed = true;
      occupation[i] = occupied;
      // String topic = "/homeassistant/binary_sensor/espRadar/zone_" + String(i + 1) + "/state";
      // String payload = occupied ? "ON" : "OFF";
      // Serial.println("Publishing to " + topic + ": " + payload);
      // client.publish(topic, payload);
      // sendMessage(i + 1, occupied);
    }
  }
  bool currentMov = radar.isMoving();
  if (currentMov != isMoving)
  {
    isMoving = currentMov;
    changed = true;
  }

  // delay(10);  // <- fixes some issues with WiFi stability

  if (!client.connected())
  {
    connect();
  }
  // client.publish("/homeassistant/device/espRadar/config", "{\"dev\": {}}");
  // publish a message roughly every second.
  // if ((millis() - lastMillis > 10000 || changed) && !isMoving)
  if (changed)
  {
    // lastMillis = millis();
    client.publish("espRadar/state", createMqttState().c_str());
  }
}