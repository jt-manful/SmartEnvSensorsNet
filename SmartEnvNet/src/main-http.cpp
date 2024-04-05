#ifndef DEVICE_ID
#define DEVICE_ID "default_id"
#endif

#include <Wire.h>
#include <SPIFFS.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include "htmltext.h"



//
const int LEDTypeID = 1;
const int  HumTypeID =  3;
const int  TempTypeID = 2;

const int NodeID = (DEVICE_ID == "indoor_sensor" ? 2 :  1);
// DHT sensor setup
#define DHTPIN 18  
#define DHTTYPE DHT22 
DHT dht(DHTPIN, DHTTYPE);

// LDR setup
const int ldrPin = 33; 

//LED setup
const int ledPin = 13;

// LCD setup
LiquidCrystal_I2C lcd(0x27, 20, 4); 

// Timing variables
unsigned long lastHumidityReadTime = 0;
unsigned long lastTemperatureAndLightReadTime = 0;
unsigned long lastSaveTime = 0;

// LED interval for blinking (in milliseconds)
const unsigned long blinkInterval = 2000; // 2 seconds
unsigned long previousMillis = 0;

//WiFi credentials and server address for sending data
const char* ssid = "JOHN-2 9490";
const char* password = "deeznuts";
const char* serverName = "http://172.16.3.44/final_project/api.php";


//set up access point
WebServer server(80);
char ssidAP[] = "JohnM";
char passwordAP[] = "qwerty123";
IPAddress local_ip(192, 168, 2, 1);
IPAddress gateway(192, 168, 2, 1);
IPAddress subnet(255, 255, 255, 0);


void handleRoot();
void handleTemperaturePage();
void handleConfigPage();
void handleUpdateConfig();

//values to post
float humidity;
float temperature;
int lightIntensity;

void sendData();
bool  setupNetwork();
void blinkLED();


void setup() {
  Serial.begin(115200);

  Serial.print("Device ID: ");
  Serial.println(DEVICE_ID);

   if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
  dht.begin();

  // Initialize the LCD
  lcd.init();
  lcd.backlight();

  pinMode(ldrPin, INPUT); 
  pinMode(ledPin, OUTPUT);

  setupNetwork();
  // setting up AP
  WiFi.mode(WIFI_AP);
  delay(1000);
  WiFi.softAP(ssidAP, passwordAP);
  WiFi.softAPConfig(local_ip, gateway, subnet);  // initialise Wi-Fi
  server.begin();

  server.on("/", HTTP_GET, handleRoot);
  server.on("/temperature", HTTP_GET, handleTemperaturePage);
  server.on("/config", HTTP_GET, handleConfigPage);
  server.on("/updateConfig", HTTP_POST, handleUpdateConfig);


  //ota stuff
   Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  ArduinoOTA.setHostname("johnM");
  ArduinoOTA.setPassword("qwerty123");
  ArduinoOTA.begin();
}

void loop() {

  unsigned long currentTime = millis();
   ArduinoOTA.handle();
  blinkLED();
  server.handleClient();
  // Read and display humidity every 3 seconds
  if (currentTime - lastHumidityReadTime >= 3000) {
     humidity = dht.readHumidity();
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
    temperature = dht.readTemperature();
    lightIntensity = analogRead(ldrPin);
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

  // Save data to SPIFFS and Database once every minute
  if (currentTime - lastSaveTime >= 10000) {
    lastSaveTime = currentTime;
    
    sendData();
    // Construct the data string to save
    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();
    int lightIntensity = analogRead(ldrPin);
    String dataString =  String(temperature) + "," + String(humidity) + "," + String(lightIntensity) + "\n";
    
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

void blinkLED() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= blinkInterval) {
    previousMillis = currentMillis;

    if (digitalRead(ledPin) == LOW) {
      digitalWrite(ledPin, HIGH);
    } else {
      digitalWrite(ledPin, LOW);
    }
  }
}

void sendData() {

  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;
    http.begin(client, serverName);
    http.addHeader("Content-Type", "application/json");
    String temp_json = "{\"NodeID\":\"" + String(NodeID)+ "\",\"TypeID\":\"" + String(TempTypeID) + "\",\"Value\":\"" + String(temperature) + "\"}";
    String hum_json =  "{\"NodeID\":\"" + String(NodeID) + "\",\"TypeID\":\"" + String(HumTypeID) + "\",\"Value\":\"" + String(humidity) + "\"}";
    String ldr_json =  "{\"NodeID\":\"" + String(NodeID) + "\",\"TypeID\":\"" + String(LEDTypeID) + "\",\"Value\":\"" + String(lightIntensity) + "\"}";

    Serial.println("jsons: ");
    Serial.println(temp_json);
    Serial.println(hum_json);
    Serial.println(ldr_json);

    int httpResponseCode = http.POST(temp_json);
    Serial.print("HTTP Response code (temp): ");
    Serial.println(httpResponseCode);
    httpResponseCode = http.POST(hum_json);
    Serial.print("HTTP Response code (hum): ");
    Serial.println(httpResponseCode);
    httpResponseCode = http.POST(ldr_json);
    Serial.print("HTTP Response code (DHT): ");
    Serial.println(httpResponseCode);
    http.end(); // Free resources

  } else {
    setupNetwork();
  }
}



// Connect or reconnect to the WiFi network
bool setupNetwork(){
  WiFi.begin(ssid, password);
  Serial.println("Connecting");

   int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 10) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  // Wait for connection
  while(WiFi.status() != WL_CONNECTED) {
    Serial.println("Failed to connect to WiFi. Please check your credentials");
    return false;

  }

  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
  return true;

}

void handleRoot() {
  // Example of dynamically replacing placeholders in your page
  String fullPage = String(page1); // Assuming page is your main page's HTML content
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