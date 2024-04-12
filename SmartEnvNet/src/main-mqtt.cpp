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
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <WiFiUdp.h>
#include <FS.h>
#include "htmltext.h"


const char* loginIndex =
 "<form name='loginForm'>"
    "<table width='20%' bgcolor='A09F9F' align='center'>"
        "<tr>"
            "<td colspan=2>"
                "<center><font size=4><b>ESP32 Login Page</b></font></center>"
                "<br>"
            "</td>"
            "<br>"
            "<br>"
        "</tr>"
        "<tr>"
             "<td>Username:</td>"
             "<td><input type='text' size=25 name='userid'><br></td>"
        "</tr>"
        "<br>"
        "<br>"
        "<tr>"
            "<td>Password:</td>"
            "<td><input type='Password' size=25 name='pwd'><br></td>"
            "<br>"
            "<br>"
        "</tr>"
        "<tr>"
            "<td><input type='submit' onclick='check(this.form)' value='Login'></td>"
        "</tr>"
    "</table>"
"</form>"
"<script>"
    "function check(form)"
    "{"
    "if(form.userid.value=='admin' && form.pwd.value=='admin')"
    "{"
    "window.open('/serverIndex')"
    "}"
    "else"
    "{"
    " alert('Error Password or Username')/*displays error message*/"
    "}"
    "}"
"</script>";

/*
 * Server Index Page
 */

const char* serverIndex =
"<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
"<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
   "<input type='file' name='update'>"
        "<input type='submit' value='Update'>"
    "</form>"
 "<div id='prg'>progress: 0%</div>"
 "<script>"
  "$('form').submit(function(e){"
  "e.preventDefault();"
  "var form = $('#upload_form')[0];"
  "var data = new FormData(form);"
  " $.ajax({"
  "url: '/update',"
  "type: 'POST',"
  "data: data,"
  "contentType: false,"
  "processData:false,"
  "xhr: function() {"
  "var xhr = new window.XMLHttpRequest();"
  "xhr.upload.addEventListener('progress', function(evt) {"
  "if (evt.lengthComputable) {"
  "var per = evt.loaded / evt.total;"
  "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
  "}"
  "}, false);"
  "return xhr;"
  "},"
  "success:function(d, s) {"
  "console.log('success!')"
 "},"
 "error: function (a, b, c) {"
 "}"
 "});"
 "});"
 "</script>";


// DHT sensor setup
#define DHTPIN 18  
#define DHTTYPE DHT22 
DHT dht(DHTPIN, DHTTYPE);

//sensor IDs
const int LEDTypeID = 1;
const int  HumTypeID =  3;
const int  TempTypeID = 2;

// LDR setup
const int ldrPin = 33; 
const int ledPin = 13;
const int fanPin = 19;
bool fanState = false;

float globalTemperature;
float globalHumidity;
int globalLightIntensity;

// LCD setup
LiquidCrystal_I2C lcd(0x27, 20, 4); 

// Timing variables
unsigned long lastHumidityReadTime = 0;
unsigned long lastTemperatureAndLightReadTime = 0;
unsigned long lastSaveTime = 0;

const char* host = "esp32";
const char* ssid = "Tsatsu";
const char* password = "tsatsu123";
const char* serverName = "http://192.168.137.41/final_project/api.php";
const char* mlServerName = "http://192.168.137.41/final_project/mldata.php";

WebServer server(80);
char ssidAP[] = "johntsatsu";
char passwordAP[] = "qwerty123";
IPAddress local_ip(192, 168, 2, 1);
IPAddress gateway(192, 168, 2, 1);
IPAddress subnet(255, 255, 255, 0);

// Update MQTT broker details with your local MQTT server
const char* mqtt_server = "192.168.137.41"; 
const int mqtt_port =1883;
const char *topic1 = "iotfinal/temp1";
const char *topic2 = "iotfinal/hum1";
const char *topic3 = "iotfinal/light1";
const char *topic4 = "iotfinal/mldata1";

const int NodeID = DEVICE_ID;


WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;

#define MSG_BUFFER_SIZE  (50)
char msg[MSG_BUFFER_SIZE];

struct DeviceConfig {
  String deviceId;
  String commMethod = "http";
  int manualOverride = 0;
  float triggerTemp;
} deviceConfig;

// LED interval for blinking (in milliseconds)
const unsigned long blinkInterval = 2000; // 2 seconds
unsigned long previousMillis = 0;

void blinkLED();
void publishMessage(const char* topic, String payload , boolean retained);
void reconnect();
void connectMQTTBroker();
void connectToWifi();
void handleRoot();
void handleConfigPage();
void handleUpdateConfig(const String& jsonStr);
void handleTemperaturePage();
void handleSensorValues(float globalTemperature, float globalHumidity, int globalLightIntensity); 
void handleLDRRecords();
void handleDataRequest();
void loadConfiguration();
void autoControlFan(float currentTemperature);
void sendData(float temperature, float humidity);
void handleUpdateDeviceId();
void sendDataHum();
void sendDataTempxLDR();
void mqttPostingHum();
void mqttPostingTempXLDR();
void MLData();

void callback(char* topic, byte* payload, unsigned int length) {
  String incommingMessage = "";
  for (int i = 0; i < length; i++) incommingMessage+=(char)payload[i];

  Serial.println("Message arrived ["+String(topic)+"]"+incommingMessage);
 
}

void setup() {
  Serial.begin(115200);
  Serial.println("Device ID: ");
  Serial.print(DEVICE_ID);
  Serial.println(" ");
  
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
  pinMode(ledPin, OUTPUT);


 
  
  connectToWifi();
  /*use mdns for host name resolution*/
  if (!MDNS.begin(host)) { //http://esp32.local
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }

  server.on("/login", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", loginIndex);
  });


  server.on("/serverIndex", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", serverIndex);
  });

  /*handling uploading firmware file */
  server.on("/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("Update: %s\n", upload.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      /* flashing firmware to ESP*/
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
    }
  });
 

  connectMQTTBroker(); 
  client.setCallback(callback);

  
  
  

  server.on("/", HTTP_GET, handleRoot);
  server.on("/temperature", HTTP_GET, handleTemperaturePage);
  server.on("/config", HTTP_GET, handleConfigPage);
  server.on("/updateDeviceId", HTTP_POST, handleUpdateDeviceId);
  server.on("/getLDRRecords", HTTP_GET, handleLDRRecords);
  server.on("/data", HTTP_GET, handleDataRequest);
  server.on("/updateConfig", HTTP_POST, []() {
    if (server.hasArg("plain") == false) {
      server.send(400, "text/plain", "Bad Request: No data received");
      return;
    }

    String formDataJson = server.arg("plain");
    handleUpdateConfig(formDataJson);
    server.send(200, "text/plain", "Configuration Updated Successfully");
  });
  server.on("/sensorValue", HTTP_GET, []() {
    handleSensorValues(globalTemperature, globalHumidity, globalLightIntensity);
  });
  
  server.on("/startFan", HTTP_GET, []() {
      Serial.println("Reached here 1");

    if (deviceConfig.manualOverride == 0) {
      Serial.println("auto reached");
        fanState = true;
        digitalWrite(fanPin, HIGH);
        server.send(200, "text/plain", "Fan started. Manual mode activated.");
        Serial.println("MAN FAN OFF");
      
    } 
  });

  server.on("/stopFan", HTTP_GET, []() {
      Serial.println("Reached here 3");

    if (deviceConfig.manualOverride == 0) {
        Serial.println("Reached here 4");

        fanState = false;
        digitalWrite(fanPin, LOW);
        server.send(200, "text/plain", "Fan stopped. Manual mode activated.");
        Serial.println("MAN FAN OFF");
      } 
    
  });

  server.begin();
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32Client-";
    clientId += String(WiFi.macAddress());
    // Attempt to connect
    if (client.connect(clientId.c_str())){
      Serial.println("Local MQTT broker connected");

      // client.subscribe(topic1);   // subscribe the topics here
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");   // Wait 5 seconds before retrying
      delay(2000);
    }
  }
}

void loop() {
  unsigned long currentTime = millis();
  server.handleClient();
  blinkLED();

  if (!client.connected()) reconnect();
  client.loop();


 if(deviceConfig.manualOverride == 1){ 
      autoControlFan(globalTemperature);
    }
  // Read and display humidity every 3 seconds
  if (currentTime - lastHumidityReadTime >= 3000) {
    globalHumidity = dht.readHumidity();
    lastHumidityReadTime = currentTime;
     // http x mqtt 
  if (deviceConfig.commMethod == "http"){
    sendDataHum();
  }
  else if (deviceConfig.commMethod == "mqtt"){
    mqttPostingHum();
  }
  else{
    sendDataHum();

  }


    // Display humidity
    lcd.setCursor(0, 0);
    lcd.print("Humidity: ");
    lcd.print(globalHumidity);
    lcd.print("%   ");

    lcd.setCursor(0, 1);
    lcd.print("                ");
    
    
  }

  // Read and display temperature and light intensity every 6 seconds
  if (currentTime - lastTemperatureAndLightReadTime >= 6000) {
    globalTemperature = dht.readTemperature();
    globalLightIntensity = analogRead(ldrPin);
    lastTemperatureAndLightReadTime = currentTime;

     // http x mqtt 
    if (deviceConfig.commMethod == "http"){
      sendDataTempxLDR();
      MLData();
    }
    else if (deviceConfig.commMethod == "mqtt"){
      mqttPostingTempXLDR();
    }
    else{
      sendDataTempxLDR();

    }

    // Display temperature
    lcd.setCursor(0, 0);
    lcd.print("Temp: ");
    lcd.print(globalTemperature);
    lcd.print("C   ");

    // Display light intensity
    lcd.setCursor(0, 1);
    lcd.print("Light: ");
    lcd.print(globalLightIntensity);
    lcd.print("   ");

    Serial.println("value");
  Serial.println(deviceConfig.deviceId+ " , "+deviceConfig.commMethod+ " , "+deviceConfig.manualOverride);
  
  }

  // Save data to SPIFFS once every minute
  if (currentTime - lastSaveTime >= 10000) {
    lastSaveTime = currentTime;
    
    String dataString =  String(globalTemperature) + "," +   String(globalHumidity) + "," +  String(globalLightIntensity) +  "\n";

    handleSensorValues(globalTemperature, globalHumidity, globalLightIntensity);

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

  }

  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
}

void publishMessage(const char* topic, String payload , boolean retained){
  if (client.publish(topic, payload.c_str(), true))
      Serial.println("Message published ["+String(topic)+"]: "+payload);
}

void handleRoot() {
  String fullPage = String(page1);
  server.send(200, "text/html", fullPage.c_str());
}

void handleTemperaturePage() {
  server.send_P(200, "text/html", page2);
}

void handleConfigPage() {
  server.send_P(200, "text/html", page3);
}

void handleUpdateConfig(const String& jsonStr) {
  // Serial.println("Reached here 1");
  if (!server.hasArg("plain")) {
    server.send(400, "text/plain", "Bad Request");
    return;
  }

  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile) {
    Serial.println("Failed to open config file for reading");
    return;
  }

  size_t size = configFile.size();
  std::unique_ptr<char[]> buf(new char[size]);
  configFile.readBytes(buf.get(), size);
  configFile.close();

  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, buf.get());

  if (error) {
    server.send(400, "text/plain", "Error parsing JSON");
    return;
  }


  //write to spiffs
  StaticJsonDocument<256> newDoc;
  error = deserializeJson(newDoc, jsonStr);
  if(error){
    server.send(400, "text/plain", "Error parsing JSON");
    return;
  }
  JsonObject newObj = newDoc.as<JsonObject>();

  for (JsonPair p : newObj) {
    doc[p.key()] = p.value();  // Update existing configuration with new values
  }

  configFile = SPIFFS.open("/config.json", FILE_WRITE);
  if (!configFile) {
    server.send(500, "text/plain", "Failed to open config file for writing");
    return;
  }
  serializeJson(doc, configFile);
  configFile.close();

  server.send(200, "text/plain", "Configuration Updated");
  loadConfiguration();
  handleUpdateDeviceId(); //change device id in db
  Serial.println("value");
  Serial.println(deviceConfig.deviceId+ " , "+deviceConfig.commMethod+ " , "+deviceConfig.manualOverride);
  
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
  String baseEndpoint = "http://192.168.137.41/final_project/viewldr25.php?NodeID=";
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
  Serial.println("page2: " + temperature + " " + humidity + "\n");

  server.send(200, "application/json", jsonResponse);
}

void autoControlFan(float currentTemperature){
    if (currentTemperature >= deviceConfig.triggerTemp) { 
      fanState = true;
      digitalWrite(fanPin, HIGH);
      // Serial.println("FAN ON");
      }else { 
        fanState = false;
        digitalWrite(fanPin, LOW);
        // Serial.println("FAN OFF");
    }
  }
 

void sendData(float temperature, float humidity) {
  if (deviceConfig.commMethod == "mqtt") {
    // Code to publish data using MQTT
  } else {
    // Code to send data using HTTP POST
  }
}

void handleUpdateDeviceId() {
  const char* apiEndpoint = "http://192.168.137.41/final_project/api.php";
  if(WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi is not connected. Cannot send device ID.");
    return;
  }

  HTTPClient http;
  http.begin(apiEndpoint);
  http.addHeader("Content-Type", "application/json");

  
  String jsonPayload = "{\"NodeID\": \"" + String(DEVICE_ID) + "\", \"NewNodeName\": \"" + deviceConfig.deviceId + "\"}";
  int httpCode = http.PUT(jsonPayload);

  if (httpCode > 0) {
    Serial.printf("Response code: %d\n", httpCode);

    String payload = http.getString();
    Serial.println("Received payload: " + payload);
  } else {
    Serial.printf("Error sending POST request: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
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

// http functions
void sendDataTempxLDR() {

  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;
    http.begin(client, serverName);
    http.addHeader("Content-Type", "application/json");
    String temp_json = "{\"NodeID\":\"" + String(NodeID)+ "\",\"TypeID\":\"" + String(TempTypeID) + "\",\"Value\":\"" + String(globalTemperature) + "\"}";
    String ldr_json =  "{\"NodeID\":\"" + String(NodeID) + "\",\"TypeID\":\"" + String(LEDTypeID) + "\",\"Value\":\"" + String(globalLightIntensity) + "\"}";

    Serial.println("jsons: ");
    Serial.println(temp_json);
    Serial.println(ldr_json);

    int httpResponseCode = http.POST(temp_json);
    Serial.print("HTTP Response code (temp): ");
    Serial.println(httpResponseCode);

    httpResponseCode = http.POST(ldr_json);
    Serial.print("HTTP Response code (DHT): ");
    Serial.println(httpResponseCode);
    http.end(); // Free resources

  } else {
    connectToWifi();
  }
}


void sendDataHum() {

  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;
    http.begin(client, serverName);
    http.addHeader("Content-Type", "application/json");
    String hum_json =  "{\"NodeID\":\"" + String(NodeID) + "\",\"TypeID\":\"" + String(HumTypeID) + "\",\"Value\":\"" + String(globalHumidity) + "\"}";

    Serial.println("jsons: ");
    Serial.println(hum_json);


    int httpResponseCode = http.POST(hum_json);
    Serial.print("HTTP Response code (hum): ");
    Serial.println(httpResponseCode);
    
    http.end(); // Free resources

  } else {
    connectToWifi();
  }
}

void mqttPostingHum(){
  String dataStringHum = String(NodeID) + "," + String(globalHumidity);

  // Publish temperature value to the specified topic
  publishMessage(topic2,dataStringHum,true);
}

void mqttPostingTempXLDR(){
  String dataStringTemp = String(NodeID) + "," + String(globalTemperature);
  String dataStringLDR = String(NodeID) + "," + String(globalLightIntensity);

  String data_json =  "{\"NodeID\":\"" + String(NodeID) + "\",\"Temperature\":\"" + String(globalTemperature) + "\",\"LDR\":\"" + String(globalLightIntensity) + "\", \"Humidity\":\"" + String(globalHumidity) + "\"}";

  publishMessage(topic3,dataStringLDR,true);  
  publishMessage(topic1,dataStringTemp,true);  
  publishMessage(topic4,data_json,true);

}


void MLData() {

  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;
    http.begin(client, mlServerName);
    http.addHeader("Content-Type", "application/json");
    String data_json =  "{\"NodeID\":\"" + String(NodeID) + "\",\"Temperature\":\"" + String(globalTemperature) + "\",\"LDR\":\"" + String(globalLightIntensity) + "\", \"Humidity\":\"" + String(globalHumidity) + "\"}";

    Serial.println("jsons: ");
    Serial.println(data_json);


    int httpResponseCode = http.POST(data_json);
    Serial.print("HTTP Response code (hum): ");
    Serial.println(httpResponseCode);
    
    http.end();

  } else {
    connectToWifi();
  }
}


