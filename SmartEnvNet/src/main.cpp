#ifndef DEVICE_ID
#define DEVICE_ID "default-id"
#endif

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
}

void loop() {
  unsigned long currentTime = millis();

  blinkLED();

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