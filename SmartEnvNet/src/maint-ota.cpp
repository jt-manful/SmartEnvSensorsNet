// #include <WiFi.h>
// #include <WiFiClient.h>
// #include <WebServer.h>
// #include <ESPmDNS.h>
// #include <Update.h>
// #include <SPIFFS.h>
// #include <LiquidCrystal_I2C.h>
// #include <Adafruit_Sensor.h>
// #include <DHT.h>
// #include <ArduinoOTA.h>
// #include <ArduinoJson.h>

// // Your global variable definitions and initializations here
// const int LEDTypeID = 1;
// const int  HumTypeID =  3;
// const int  TempTypeID = 2;

// const int NodeID = (DEVICE_ID == "indoor_sensor" ? 2 :  1);
// // DHT sensor setup
// #define DHTPIN 18  
// #define DHTTYPE DHT22 
// DHT dht(DHTPIN, DHTTYPE);

// // LDR setup
// const int ldrPin = 33; 

// //LED setup
// const int ledPin = 13;

// // LCD setup
// LiquidCrystal_I2C lcd(0x27, 20, 4); 

// // Timing variables
// unsigned long lastHumidityReadTime = 0;
// unsigned long lastTemperatureAndLightReadTime = 0;
// unsigned long lastSaveTime = 0;

// // LED interval for blinking (in milliseconds)
// const unsigned long blinkInterval = 2000; // 2 seconds
// unsigned long previousMillis = 0;


// //set up access point
// WebServer server(80);
// char ssidAP[] = "JohnM";
// char passwordAP[] = "qwerty123";
// IPAddress local_ip(192, 168, 2, 1);
// IPAddress gateway(192, 168, 2, 1);
// IPAddress subnet(255, 255, 255, 0);


// void handleRoot();
// void handleTemperaturePage();
// void handleConfigPage();
// void handleUpdateConfig();

// const char* host = "esp32";
// const char* ssid = "JOHN-2 9490";
// const char* password = "deeznuts";
// const char* serverName = "http://172.16.3.44/final_project/api.php";

// float humidity;
// float temperature;
// int lightIntensity;

// void sendData();
// bool setupNetwork();
// void blinkLED();




// /*
//  * Login page
//  */

// const char* loginIndex =
//  "<form name='loginForm'>"
//     "<table width='20%' bgcolor='A09F9F' align='center'>"
//         "<tr>"
//             "<td colspan=2>"
//                 "<center><font size=4><b>ESP32 Login Page</b></font></center>"
//                 "<br>"
//             "</td>"
//             "<br>"
//             "<br>"
//         "</tr>"
//         "<tr>"
//              "<td>Username:</td>"
//              "<td><input type='text' size=25 name='userid'><br></td>"
//         "</tr>"
//         "<br>"
//         "<br>"
//         "<tr>"
//             "<td>Password:</td>"
//             "<td><input type='Password' size=25 name='pwd'><br></td>"
//             "<br>"
//             "<br>"
//         "</tr>"
//         "<tr>"
//             "<td><input type='submit' onclick='check(this.form)' value='Login'></td>"
//         "</tr>"
//     "</table>"
// "</form>"
// "<script>"
//     "function check(form)"
//     "{"
//     "if(form.userid.value=='admin' && form.pwd.value=='admin')"
//     "{"
//     "window.open('/serverIndex')"
//     "}"
//     "else"
//     "{"
//     " alert('Error Password or Username')/*displays error message*/"
//     "}"
//     "}"
// "</script>";

// /*
//  * Server Index Page
//  */

// const char* serverIndex =
// "<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
// "<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
//    "<input type='file' name='update'>"
//         "<input type='submit' value='Update'>"
//     "</form>"
//  "<div id='prg'>progress: 0%</div>"
//  "<script>"
//   "$('form').submit(function(e){"
//   "e.preventDefault();"
//   "var form = $('#upload_form')[0];"
//   "var data = new FormData(form);"
//   " $.ajax({"
//   "url: '/update',"
//   "type: 'POST',"
//   "data: data,"
//   "contentType: false,"
//   "processData:false,"
//   "xhr: function() {"
//   "var xhr = new window.XMLHttpRequest();"
//   "xhr.upload.addEventListener('progress', function(evt) {"
//   "if (evt.lengthComputable) {"
//   "var per = evt.loaded / evt.total;"
//   "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
//   "}"
//   "}, false);"
//   "return xhr;"
//   "},"
//   "success:function(d, s) {"
//   "console.log('success!')"
//  "},"
//  "error: function (a, b, c) {"
//  "}"
//  "});"
//  "});"
//  "</script>";



// void setup() {
//   Serial.begin(115200);

//   Serial.print("Device ID: ");
//   Serial.println(DEVICE_ID);

//   // Setup code from both OTA and main application here
//   WiFi.begin(ssid, password);
//   while (WiFi.status() != WL_CONNECTED) {
//     delay(500);
//     Serial.print(".");
//   }
//   Serial.println("Connected to WiFi");

//   if (!SPIFFS.begin(true)) {
//     Serial.println("An Error has occurred while mounting SPIFFS");
//     return;
//   }

//   dht.begin();
//   lcd.init();
//   lcd.backlight();
//   pinMode(ldrPin, INPUT);
//   pinMode(ledPin, OUTPUT);

//   // Additional setup for OTA and server routes
  
// /*use mdns for host name resolution*/
//   if (!MDNS.begin(host)) { //http://esp32.local
//     Serial.println("Error setting up MDNS responder!");
//     while (1) {
//       delay(1000);
//     }
//   }
//   Serial.println("mDNS responder started");
//   /*return index page which is stored in serverIndex */
//   server.on("/", HTTP_GET, []() {
//     server.sendHeader("Connection", "close");
//     server.send(200, "text/html", loginIndex);
//   });
//   server.on("/serverIndex", HTTP_GET, []() {
//     server.sendHeader("Connection", "close");
//     server.send(200, "text/html", serverIndex);
//   });
//   /*handling uploading firmware file */
//   server.on("/update", HTTP_POST, []() {
//     server.sendHeader("Connection", "close");
//     server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
//     ESP.restart();
//   }, []() {
//     HTTPUpload& upload = server.upload();
//     if (upload.status == UPLOAD_FILE_START) {
//       Serial.printf("Update: %s\n", upload.filename.c_str());
//       if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
//         Update.printError(Serial);
//       }
//     } else if (upload.status == UPLOAD_FILE_WRITE) {
//       /* flashing firmware to ESP*/
//       if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
//         Update.printError(Serial);
//       }
//     } else if (upload.status == UPLOAD_FILE_END) {
//       if (Update.end(true)) { //true to set the size to the current progress
//         Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
//       } else {
//         Update.printError(Serial);
//       }
//     }
//   });
  
//   server.begin(); // Start the web server
// }

// void loop() {
//   // Loop code from both OTA and main application here
//   ArduinoOTA.handle();
//   server.handleClient();
  
//   // Your sensor reading or actuator control logic here
// }