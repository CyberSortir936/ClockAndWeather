#include <Arduino.h>
#include "DHT.h"
#include "Arduino_JSON.h"
#include <WiFi.h>
#include <Wire.h>
#include <HTTPClient.h>
#include "../lib/credential.h"
#include "Adafruit_SSD1306.h"
#include "Adafruit_GFX.h"
#include "time.h"
#include "../lib/weather_bitmaps.h"

#define BUTTON_PIN 4

#define SDA_PIN 21
#define SCL_PIN 22
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64  

#define OLED_RESET     -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//DHT config
#define DHTPIN 5
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// WeatherAPI config
String City = "Lviv";
String CountryCode = "UA";
String jsonBuffer;

// NTP server config
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 7200;
const int   daylightOffset_sec = 0;

// Weather and time variables
float in_temperature = 0;
float in_humidity = 0;

String out_temperature = "0";
String out_humidity = "0";
String skyState = "Sun";

char hour[3] = "0";
char minute[3] = "0";

bool isConnected = true;
bool dotsFlag = true;
char previousMinutes[3] = "0";
unsigned long startTime = 0;
unsigned long timerDelay = 10000;

String httpGETRequest(const char* serverName);
void updateInData();
void updateOutData();
void updateLocalTime();
bool isItTimetoUpdate();

void setup() {
  Serial.begin(115200);
  Serial.println(F("DHTxx test!"));
  dht.begin();

  Wire.begin(SDA_PIN, SCL_PIN);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Wire.begin(SDA_PIN, SCL_PIN);

  // Check if the OLED is actually on the bus
  Wire.beginTransmission(0x3C);
  if (Wire.endTransmission() != 0) {
    Serial.println("OLED not found at 0x3C! Check wiring/power.");
    // You can choose to loop here or skip OLED code
  } else {
    Serial.println("OLED found!");
  }

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Connecting");
  display.setCursor(10, 16);

  WiFi.begin(ssid, pass);
  Serial.println("Connecting");
  startTime = millis();
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");

    display.print(".");
    display.display();

    unsigned long connectionTime = millis();

    if((connectionTime - startTime) > timerDelay){
      isConnected = false;
      break;
    }
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  updateInData();
  // updateOutData();
  updateLocalTime();
  display.clearDisplay();
  display.display();
  delay(1000);
}

void loop() {
  byte buttonState = digitalRead(BUTTON_PIN);
  updateInData();

  if(isConnected){
    if (buttonState == LOW || isItTimetoUpdate()) {
    updateInData();
    updateLocalTime();
    }
  }
  else{
    display.setCursor(16, 0);
    display.println("NO WIFI");
    display.println();
    display.print(in_temperature);
    display.print(" C");

    display.println();
    display.print(in_humidity);
    display.print(" %");
    display.display();
  }

  Serial.println();
  Serial.print("In Temp: ");
  Serial.print(in_temperature);
  Serial.println();
  Serial.print("In Humidity: ");
  Serial.print(in_humidity);
  Serial.println();

  Serial.print("Out Temp: ");
  Serial.print(out_temperature);
  Serial.println();
  Serial.print("Out Humidity: ");
  Serial.print(out_humidity);
  Serial.println();
  Serial.print("Sky State: ");
  Serial.print(skyState);
  Serial.println();

  Serial.print(hour);
  Serial.print(" : ");
  Serial.print(minute);
  delay(1000);
}

// update outer weather info every 30 minutes
bool isItTimetoUpdate(){
  if(minute == "30" || minute == "00"){
    if(minute != previousMinutes){
      return true;
    }
    else return false;
  }
  else return false;
}
String httpGETRequest(const char* serverName) {
  WiFiClient client;
  HTTPClient http;
    
  // Your Domain name with URL path or IP address with path
  http.begin(client, serverName);
  
  // Send HTTP POST request
  int httpResponseCode = http.GET();
  
  String payload = "{}"; 
  
  if (httpResponseCode>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

  return payload;
}
void updateInData(){
  in_humidity = dht.readHumidity();
  in_temperature = dht.readTemperature();

  // Check if any reads failed and exit early (to try again).
  if (isnan(in_humidity) || isnan(in_temperature)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }
}
void updateOutData(){
  // Check WiFi connection status
    if(WiFi.status()== WL_CONNECTED){
      String serverPath = "http://api.openweathermap.org/data/2.5/weather?q=" + City + "," + CountryCode + "&APPID=" + weather_api + "&units=metric";
      
      jsonBuffer = httpGETRequest(serverPath.c_str());
      //Serial.println(jsonBuffer);
      JSONVar myObject = JSON.parse(jsonBuffer);
  
      // JSON.typeof(jsonVar) can be used to get the type of the var
      if (JSON.typeof(myObject) == "undefined") {
        Serial.println("Parsing input failed!");
        return;
      }
    

      out_temperature = JSON.stringify(myObject["main"]["temp"]);
      out_humidity = JSON.stringify(myObject["main"]["humidity"]);
      skyState = JSON.stringify(myObject["weather"][0]["description"]);
    }
    else {
      Serial.println("WiFi Disconnected");
    }
}
void updateLocalTime(){
  
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  strftime(hour,3, "%H", &timeinfo);
  strftime(minute,3, "%M", &timeinfo);
}