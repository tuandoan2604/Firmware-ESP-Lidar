#include <WiFi.h>
#include <HardwareSerial.h>
#include <WebServer.h>
#include "Ticker.h"
#include "peripherial.h"
#include <RPLidar.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include <AsyncTCP.h>
#include "ESPAsyncWebServer.h"


DynamicJsonDocument doc(128);
AsyncWebServer server(80);
bool state = false;
extern String ssidST;
extern String passwordST;
RPLidar lidar;
float distance;
float angle;
#define RPLIDAR_MOTOR 21
const int freq = 5000;
const int ledChannel = 0;
const int resolution = 8;

/*-- Timeout status sensor --*/
unsigned long _start_throw_time_ = 0;
unsigned long _interval_time_ = 10000;

/*-- Update vibration sensor --*/
#define SENSOR_PIN    5           // Init sensor pin
#define SENSOR_MAX_INPUT   50    // Max input vibration
int sensor_count = 0;             // Sensor input counting
bool sensor_status = false;       // Status

String processor(const String &var)
{
  if (var == "DISTANCECHANGE")
  {
  }
  else if (var == "ANGLECHANGE")
  {
  }
  return String();
}

// String tranferData() {
//   Serial.println("Start scan data Lirad");
//   if (distance > 300) {
//     distance = 300; angle = 0;
//   }
//   doc["distance"] = distance;
//   doc["angle"] = angle;
//   String output;
//   serializeJsonPretty(doc,output);
//   return output;
//   //server.send(200, "application/json", output);
// }

/*-- Get value of vibration sensor --*/
/* ------------- Interupt ------------ */
void IRAM_ATTR check_sensor()
{
  _start_throw_time_ = millis();
  sensor_count ++ ;
  if(sensor_count > SENSOR_MAX_INPUT)
  {
    sensor_status = true;
    sensor_count = 0;
  }
} 

void setup()
{
  Serial.begin(115200);
  Serial2.begin(115200);
  lidar.begin(Serial2);
  pinMode(LED_STA_PIN, OUTPUT);
  pinMode(BUTTON_ERASE,INPUT_PULLUP);
  pinMode(SENSOR_PIN, INPUT_PULLUP);                    //
  attachInterrupt(SENSOR_PIN, check_sensor, FALLING);   //
  ledcSetup(ledChannel, freq, resolution);
  ledcAttachPin(RPLIDAR_MOTOR, ledChannel);
  WiFi.mode(WIFI_STA);
  NVS_FLASH_INIT(512);
  fnreadEepromWifi();
  if (!ssidST.equals("0"))
  {
    WiFi.begin((const char *)ssidST.c_str(), (const char *)passwordST.c_str());
    char retry_connect_num = 0;
    while (WiFi.status() != WL_CONNECTED)
    {
      Serial.print('.');
      retry_connect_num++;
      digitalWrite(LED_STA_PIN, !digitalRead(LED_STA_PIN));
      delay(150);
      if (retry_connect_num > 5)
      {
        Serial.println("Connect Fail");
        WiFi.beginSmartConfig();
      }
    }
    digitalWrite(LED_STA_PIN, 0);
    Serial.println("...............WIFI CONNECTED..................");
  }
  else
  {
    Serial.println("...............START SMARTCONFIG..................");
    WiFi.beginSmartConfig();
    while (WiFi.status() != WL_CONNECTED)
    {
      digitalWrite(LED_STA_PIN, !digitalRead(LED_STA_PIN));
      delay(150);
    }
  }
  digitalWrite(LED_STA_PIN, 1);
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.SSID());
  Serial.println(WiFi.psk());
  ssidST = WiFi.SSID();
  passwordST = WiFi.psk();
  Serial.println("SSID : " + ssidST);
  fnwriteEepromWifi();
  if (MDNS.begin("esplidar"))
  {
    Serial.println("MDNS responder started");
  }
  Serial.print("IPAddress : ");
  Serial.println(WiFi.localIP());
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "HELLO LIDAR A1");
  });
  server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request) {
    Serial.println("Start scan data Lirad");
    if (distance > 3300)
    {
      distance = 3300;
      angle = 0;
    }
    /*-- Send value of vibration sensor to Webserver --*/
    doc["distance"] = distance;
    doc["angle"] = angle;
    
    String output;
    serializeJsonPretty(doc, output);
    request->send(200, "application/json", output);
 
  });
  server.begin();
  Serial.println("HTTP server started");
  MDNS.addService("http", "tcp", 80);
  ledcWrite(RPLIDAR_MOTOR, 255);
}
void loop()
{
  if (IS_OK(lidar.waitPoint()))
  {
    delay(150);
    if (sensor_status == true)
    {
      distance = 100;
    }
    else distance = lidar.getCurrentPoint().distance;      //distance value in mm unit
    angle = lidar.getCurrentPoint().angle;            //anglue value in degree
    bool startBit = lidar.getCurrentPoint().startBit; //whether this point is belong to a new scan
    byte quality = lidar.getCurrentPoint().quality;   //quality of the current measurement
//    Serial.println("_____________________________________");
//    Serial.println("Lidar Get info...");
  }
  else
  {
    ledcWrite(RPLIDAR_MOTOR, 0);
    rplidar_response_device_info_t info;
    if (IS_OK(lidar.getDeviceInfo(info, 100)))
    {
      lidar.startScan();
      ledcWrite(RPLIDAR_MOTOR, 255);
      delay(1000);
    }
  }
//  Serial.println(lidar.getCurrentPoint().distance);
//  Serial.println(lidar.getCurrentPoint().angle);
  if(BUTTON_ERASE == 0){
    clearNVSFlash();
  }
  Serial.println(millis()- _start_throw_time_);
  if((millis() - _start_throw_time_) >= _interval_time_ ) {
    sensor_status = false;
  }
//  Serial.println(lidar.getCurrentPoint().startBit);
//  Serial.println(lidar.getCurrentPoint().quality);
//  Serial.println("_____________________________________");
}
