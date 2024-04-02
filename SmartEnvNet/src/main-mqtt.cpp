// #include "DHT.h"
// #include <PubSubClient.h>
// #include <WiFi.h>

// #define DHTPIN 18
// #define DHTTYPE DHT22

// DHT dht(DHTPIN, DHTTYPE);

// const char* ssid = "Tsatsu";
// const char* password = "tsatsu123";

// // Update MQTT broker details with your local MQTT server
// const char *mqtt_broker = "172.16.7.215";
// const char *topic = "iotlab6/test";
// const int mqtt_port = 1883;

// WiFiClient espClient;
// PubSubClient client(espClient);

// void setup() {
//   Serial.begin(115200);

//   WiFi.begin(ssid, password);
//   while (WiFi.status() != WL_CONNECTED){
//     delay(500);
//     Serial.println("Connecting to WiFi...");
//   }

//   Serial.println("Connected to the WiFi network");
  
//   //connecting to the local MQTT broker
//   client.setServer(mqtt_broker, mqtt_port);
//   while (!client.connected()) {
//       String client_id = "clientId-";
//       client_id += String(WiFi.macAddress());
//       Serial.printf("The client %s connects to the local mqtt broker\n", client_id.c_str());
//       if (client.connect(client_id.c_str())) {
//           Serial.println("Local MQTT broker connected");
//       } else {
//           Serial.print("failed with state ");
//           Serial.print(client.state());
//           delay(2000);
//       }
//   }

//   dht.begin();
// }

// void loop() {
//   float t = dht.readTemperature();

//   // Check if any reads failed and exit early (to try again).
//   if (isnan(t)) {
//     Serial.println(F("Failed to read from DHT sensor!"));
//     return;
//   }

//   // Publish temperature value to the specified topic
//   client.publish(topic, String(t).c_str());
//   delay(2000);

//   Serial.print(F("Temperature: "));
//   Serial.println(t);
// }


#include <Wire.h>
#include <SPIFFS.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>

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

void setup() {
  Serial.begin(115200);
   if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  dht.begin();

  // Initialize the LCD
  lcd.init();
  lcd.backlight();

  pinMode(ldrPin, INPUT); 
}

void loop() {
  unsigned long currentTime = millis();

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
  if (currentTime - lastSaveTime >= 60000) {
    lastSaveTime = currentTime;
    
    // Construct the data string to save
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();
    int lightIntensity = analogRead(ldrPin);
    String dataString = String(currentTime) + "," + String(temperature) + "," + String(humidity) + "," + String(lightIntensity) + "\n";
    
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
    
}