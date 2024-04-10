#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <Wire.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <ArduinoJson.h>
#include "textota.h"

const char* host = "esp32";
const char* ssid = "JOHN-2 9490";
const char* password = "deeznuts";
const char* serverName = "http://172.16.4.206/final_project/api.php";


//set up access point
WebServer server(80);
char ssidAP[] = "JohnM";
char passwordAP[] = "qwerty123";
IPAddress local_ip(192, 168, 2, 1);
IPAddress gateway(192, 168, 2, 1);
IPAddress subnet(255, 255, 255, 0);


void setupWifi();
void handleRoot();
void handleTemperaturePage();
void handleConfigPage();
void handleUpdateConfig();



void setup(void) {
  Serial.begin(115200);
//192.168.1.148
  setupWifi();
  // /*use mdns for host name resolution*/
  // if (!MDNS.begin(host)) { //http://esp32.local
  //   Serial.println("Error setting up MDNS responder!");
  //   while (1) {
  //     delay(1000);
  //   }
  // }
  // Serial.println("mDNS responder started");
  /*return index page which is stored in serverIndex */

   WiFi.softAP(ssidAP, passwordAP);
  WiFi.softAPConfig(local_ip, gateway, subnet);  // initialise Wi-Fi
  server.begin();
  
  server.on("/", HTTP_GET, []() {
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

  server.on("/homepage", HTTP_GET, handleRoot);
  server.on("/temperature", HTTP_GET, handleTemperaturePage);
  server.on("/config", HTTP_GET, handleConfigPage);
  server.on("/updateConfig", HTTP_POST, handleUpdateConfig);


  server.begin();
}

void loop(void) {
  server.handleClient();
  delay(1);
}


void setupWifi(){
     // Connect to WiFi network
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

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