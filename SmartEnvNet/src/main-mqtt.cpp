#ifndef DEVICE_ID
#define DEVICE_ID "default-id"
#endif

#include <Wire.h>
#include <SPIFFS.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <WiFi.h>
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

// LCD setup
LiquidCrystal_I2C lcd(0x27, 20, 4); 

// Timing variables
unsigned long lastHumidityReadTime = 0;
unsigned long lastTemperatureAndLightReadTime = 0;
unsigned long lastSaveTime = 0;

const char* ssid = "Tsatsu";
const char* password = "tsatsu123";

// Update MQTT broker details with your local MQTT server
const char* mqtt_server = "192.168.137.41"; 
const int mqtt_port =1883;
const char *topic1 = "iotfinal/temp1";
const char *topic2 = "iotfinal/hum1";
const char *topic3 = "iotfinal/light1";


extern const char page1[];
extern const char page2[];
extern const char page3[];


WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;

#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];

void publishMessage(const char* topic, String payload , boolean retained);
void reconnect();
void connectMQTTBroker();
void connectToWifi();

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

  server.begin();

  connectToWifi();
  connectMQTTBroker(); 
  client.setCallback(callback);


  server.on("/", HTTP_GET, handleRoot);
  server.on("/temperature", HTTP_GET, handleTemperaturePage);
  server.on("/config", HTTP_GET, handleConfigPage);
  server.on("/updateConfig", HTTP_POST, handleUpdateConfig);
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
    String dataString = String(temperature) + "," + String(humidity) + "," + String(lightIntensity) + "\n";

  // Publish temperature value to the specified topic
      publishMessage(topic1,dataString,true);    
    // client.publish(topic1, String(temperature).c_str(), true);
    // client.publish(topic2, String(humidity).c_str());
    // client.publish(topic3, String(lightIntensity).c_str());
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

  Serial.println("Connected to the WiFi network");
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
  // This function needs to read the POST data and update your configuration accordingly
  if (server.hasArg("plain") == false) {
    server.send(400, "text/plain", "Bad Request");
    return;
  }

  StaticJsonDocument<512> doc;
  deserializeJson(doc, server.arg("plain"));

  // Handle your configuration update here, possibly saving to SPIFFS

  server.send(200, "text/plain", "Configuration Updated");
}