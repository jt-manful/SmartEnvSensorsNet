#ifndef DEVICE_ID
#define DEVICE_ID "default-id"
#endif

#include <Wire.h>
#include <SPIFFS.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include "htmltext.h"

// DHT sensor setup
#define DHTPIN 18  
#define DHTTYPE DHT22 
DHT dht(DHTPIN, DHTTYPE);

// LDR setup
const int ldrPin = 33; 
const int fanPin = 19;
bool fanState = false;
bool forceAction = false;
bool forceState = false;

float globalTemperature;
float globalHumidity;
int globalLightIntensity;

// LCD setup
LiquidCrystal_I2C lcd(0x27, 20, 4); 
WebServer server(80);

// Timing variables
unsigned long lastHumidityReadTime = 0;
unsigned long lastTemperatureAndLightReadTime = 0;
unsigned long lastSaveTime = 0;

const char* ssid = "JOHN-2 9490";
const char* password = "deeznuts";

char ssidAP[] = "johntsatsu";
char passwordAP[] = "qwerty123";
IPAddress local_ip(192, 168, 2, 1);
IPAddress gateway(192, 168, 2, 1);
IPAddress subnet(255, 255, 255, 0);

// Update MQTT broker details with your local MQTT server
const char* mqtt_server = "172.16.4.206"; 
const int mqtt_port =1883;
const char *topic1 = "iotfinal/temp1";
const char *topic2 = "iotfinal/hum1";
const char *topic3 = "iotfinal/light1";

const int NodeID = (DEVICE_ID == "indoor_sensor" ? 2 : 1);


WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;

#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];

struct DeviceConfig {
  String deviceId;
  String commMethod;
  bool manualOverride;
  int triggerTemp;
} deviceConfig;

void publishMessage(const char* topic, String payload , boolean retained);
void reconnect();
void connectMQTTBroker();
void connectToWifi();
void handleRoot();
void handleConfigPage();
void handleUpdateConfig();
void handleTemperaturePage();
void handleSensorValues(float globalTemperature, float globalHumidity, int globalLightIntensity); 
void handleLDRRecords();
void handleDataRequest();
void loadConfiguration();
void autoControlFan(bool forceAction, bool forceState, float currentTemperature);
void sendData(float temperature, float humidity);

void callback(char* topic, byte* payload, unsigned int length) {
  String incommingMessage = "";
  for (int i = 0; i < length; i++) incommingMessage+=(char)payload[i];
  
  Serial.println("Message arrived ["+String(topic)+"]"+incommingMessage);
 
}

void setup() {
  Serial.begin(115200);
  Serial.println("Device ID: " DEVICE_ID);
   if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  dht.begin();

  // Initialize the LCD
  lcd.init();
  lcd.backlight();

  pinMode(ldrPin, INPUT);
  pinMode(fanPin, OUTPUT);

  WiFi.mode(WIFI_AP);
  delay(1000);

  WiFi.softAP(ssidAP, passwordAP);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  
  server.begin();

  connectToWifi();
  connectMQTTBroker(); 
  client.setCallback(callback);


  server.on("/", HTTP_GET, handleRoot);
  server.on("/temperature", HTTP_GET, handleTemperaturePage);
  server.on("/config", HTTP_GET, handleConfigPage);
  server.on("/updateConfig", HTTP_POST, handleUpdateConfig);
  server.on("/getLDRRecords", HTTP_GET, handleLDRRecords);
  server.on("/sensorValue", HTTP_GET, []() {
    handleSensorValues(globalTemperature, globalHumidity, globalLightIntensity);
  });
  
  server.on("/startFan", HTTP_GET, []() {
  if (deviceConfig.manualOverride) {
    fanState = true; // Turn fan on
    digitalWrite(fanPin, HIGH);
    server.send(200, "text/plain", "Fan started");
  } else {
    autoControlFan(forceAction=true, forceState=true, globalTemperature);
    server.send(200, "text/plain", "Manual control is disabled. Operating in auto mode.");
  }
  });

  server.on("/stopFan", HTTP_GET, []() {
    if (deviceConfig.manualOverride) {
      fanState = false; // Turn fan off
      digitalWrite(fanPin, LOW);
      server.send(200, "text/plain", "Fan stopped");
    } else {
      autoControlFan(forceAction=false, forceState=false, globalTemperature);
      server.send(200, "text/plain", "Manual control is disabled. Operating in auto mode.");
    }
  });
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32Client-";   // Create a random client ID
    clientId += String(random(0xffff), HEX);  //you could make this static
    // Attempt to connect
    if (client.connect(clientId.c_str())){//, mqtt_username, mqtt_password)) {
      Serial.println("connected");

      client.subscribe(topic1);   // subscribe the topics here
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");   // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop() {
  unsigned long currentTime = millis();
  server.handleClient();
  if (!client.connected()) reconnect();
  client.loop();

  // Read and display humidity every 3 seconds
  if (currentTime - lastHumidityReadTime >= 3000) {
    float humidity = dht.readHumidity();
    lastHumidityReadTime = currentTime;

    // Display humidity
    lcd.setCursor(0, 0);
    lcd.print("Humidity: ");
    lcd.print(humidity);
    lcd.print("%   ");

    lcd.setCursor(0, 1);
    lcd.print("                ");
    
    
  }

  // Read and display temperature and light intensity every 6 seconds
  if (currentTime - lastTemperatureAndLightReadTime >= 6000) {
    float temperature = dht.readTemperature();
    int lightIntensity = analogRead(ldrPin);
    lastTemperatureAndLightReadTime = currentTime;

    // Display temperature
    lcd.setCursor(0, 0);
    lcd.print("Temp: ");
    lcd.print(temperature);
    lcd.print("C   ");

    // Display light intensity
    lcd.setCursor(0, 1);
    lcd.print("Light: ");
    lcd.print(lightIntensity);
    lcd.print("   ");
  }

  // Save data to SPIFFS once every minute
  if (currentTime - lastSaveTime >= 10000) {
    lastSaveTime = currentTime;
    
    // Construct the data string to save
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();
    int lightIntensity = analogRead(ldrPin);
    String dataString =  String(temperature) + "," +   String(humidity) + "," +  String(lightIntensity) +  "\n";

    String dataStringTemp = String(NodeID) + "," + String(temperature);
    String dataStringHum = String(NodeID) + "," + String(humidity);
    String dataStringLDR = String(NodeID) + "," + String(lightIntensity);

    handleSensorValues(temperature, humidity, lightIntensity);

    if(!deviceConfig.manualOverride){
      autoControlFan(false, false, temperature);
    }

    delay(1000);

    // Publish temperature value to the specified topic
      publishMessage(topic1,dataStringTemp,true);  
      publishMessage(topic2,dataStringHum,true);  
      publishMessage(topic3,dataStringLDR,true);    
    
    delay(2000);
    
    // Open file for appending
    File file = SPIFFS.open("/sensor_data.txt", FILE_APPEND);
    if(!file){
      Serial.println("There was an error opening the file for appending");
      return;
    }
    
    if(file.print(dataString)){
      Serial.println("Data saved: " + dataString);
    } else {
      Serial.println("Write failed");
    }

    // Always close the file when you're done with it
    file.close();
  }


}

void connectMQTTBroker(){
  //connecting to the local MQTT broker
  client.setServer(mqtt_server, mqtt_port);
  while (!client.connected()) {
      String client_id = "clientId-";
      client_id += String(WiFi.macAddress());
      Serial.printf("The client %s connects to the local mqtt broker\n", client_id.c_str());
      if (client.connect(client_id.c_str())) {
          Serial.println("Local MQTT broker connected");
      } else {
          Serial.print("failed with state ");
          Serial.print(client.state());
          delay(2000);
      }
  }
}

void connectToWifi(){
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
}

void publishMessage(const char* topic, String payload , boolean retained){
  if (client.publish(topic, payload.c_str(), true))
      Serial.println("Message publised ["+String(topic)+"]: "+payload);
}

void handleRoot() {
  // Example of dynamically replacing placeholders in your page
  String fullPage = String(page1); // Assuming `page` is your main page's HTML content
  // fullPage.replace("<!--PLACEHOLDER-->", "Actual Value");
  server.send(200, "text/html", fullPage.c_str());
}

void handleTemperaturePage() {
  server.send_P(200, "text/html", page2);
}

void handleConfigPage() {
  server.send_P(200, "text/html", page3);
}

void handleUpdateConfig() {
  if (!server.hasArg("plain")) {
    server.send(400, "text/plain", "Bad Request");
    return;
  }

  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, server.arg("plain"));

  if (error) {
    server.send(400, "text/plain", "Error parsing JSON");
    return;
  }

  File file = SPIFFS.open("/config.json", FILE_WRITE);
  if (!file) {
    server.send(500, "text/plain", "Failed to open config file for writing");
    return;
  }

  serializeJson(doc, file);
  file.close();

  server.send(200, "text/plain", "Configuration Updated");
  loadConfiguration();
}

void loadConfiguration() {
  File configFile = SPIFFS.open("/config.json", FILE_READ);
  if (!configFile) {
    Serial.println("Failed to open config file");
    return;
  }

  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, configFile);
  if (error) {
    Serial.println("Failed to parse config file");
    return;
  }

  deviceConfig.deviceId = doc["deviceId"].as<String>();
  deviceConfig.commMethod = doc["commMethod"].as<String>();
  deviceConfig.manualOverride = doc["manualOverride"].as<bool>();
  deviceConfig.triggerTemp = doc["triggerTemp"].as<int>();

  //TODO: use the loaded values later

  configFile.close();
}

void handleSensorValues(float temperature, float humidity, int lightIntensity) {
  StaticJsonDocument<200> doc;
  doc["temperature"] = temperature;
  doc["humidity"] = humidity;
  doc["lightIntensity"] = lightIntensity;

  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void handleLDRRecords() {
  String baseEndpoint = "http://172.16.4.206/final_project/viewldr25.php?NodeName=";
  String lastEndpointCStr = baseEndpoint + DEVICE_ID;

  const char* lastEndpoint = lastEndpointCStr.c_str();
  //Check WiFi connection status
    if(WiFi.status()== WL_CONNECTED){
      HTTPClient http;      
      String serverPath = lastEndpoint;
      // Your Domain name with URL path or IP address with path
      http.begin(serverPath.c_str());
      
      // Send HTTP GET request
      int httpResponseCode = http.GET();
      
      if (httpResponseCode>0) {
        // Serial.print("HTTP Response code: ");
        // Serial.println(httpResponseCode);
        String payload = http.getString();
        server.send(200, "text/html", payload);
        Serial.println(payload);
      }
      else {
        Serial.print("Error code: ");
        Serial.println(httpResponseCode);
      }
      // Free resources
      http.end();
    }
    else {
      Serial.println("WiFi Disconnected");
    }
}

void handleDataRequest() {
  // Assuming you open the file and read data as before
  File file = SPIFFS.open("/sensor_data.txt", FILE_READ);
  if (!file) {
    Serial.println("Failed to open file for reading");
    server.send(500, "application/json", "{\"error\":\"Could not read data\"}");
    return;
  }

  String lastLine;
  while (file.available()) {
    lastLine = file.readStringUntil('\n');
  }
  file.close();

  // Split the lastLine into components and send as JSON
  int firstCommaIndex = lastLine.indexOf(',');
  int secondCommaIndex = lastLine.indexOf(',', firstCommaIndex + 1);

  String temperature = lastLine.substring(0, firstCommaIndex);
  String humidity = lastLine.substring(firstCommaIndex + 1, secondCommaIndex);
  // Assuming you only want temperature and humidity for now
  
  String jsonResponse = "{\"temperature\":" + temperature + ",\"humidity\":" + humidity + "}";

  server.send(200, "application/json", jsonResponse);
}

void autoControlFan(bool forceAction, bool forceState, float currentTemperature){

  if (!deviceConfig.manualOverride) {
    if (currentTemperature >= deviceConfig.triggerTemp || forceAction && forceState) {
      if (!fanState) { // If the fan is not already on, turn it on
        fanState = true;
        digitalWrite(fanPin, HIGH);
        // Log or send a message indicating the fan was turned on automatically or due to a forced action

      }
    } else if (fanState || (forceAction && !forceState)) { // Conditions for turning off the fan
      fanState = false;
      digitalWrite(fanPin, LOW);
      // Log or send a message indicating the fan was turned off
    }
  }
  // Optionally, handle cases where manualOverride is true if needed
}

void sendData(float temperature, float humidity) {
  if (deviceConfig.commMethod == "mqtt") {
    // Code to publish data using MQTT
  } else {
    // Code to send data using HTTP POST
  }
}