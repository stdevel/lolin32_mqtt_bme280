/* Project ESP32, BME280, MQTT and Deepsleep */
#include <WiFi.h>
#include <PubSubClient.h>
#include "Wire.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "wlan.h"
#include "mqtt.h"

/* WLAN and MQTT configuration in wlan.h and mqtt.h */

/* definitions for deep sleep */
#define uS_TO_S_FACTOR 1000000    /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP 900         /* Time ESP32 will go to sleep for 15 minutes (in seconds) */
#define TIME_TO_SLEEP_ERROR 3600  /* Time to sleep in case of error (1 hour) */

/* bme280 definitions */
#define SEALEVELPRESSURE_HPA (1013.25)
/* THIS IS NEEDED FOR Lolin32 Lite ONLY */
/* for normal ESP32 boards, use pins 21 + 22 */
#define I2C_SDA_PIN 23
#define I2C_SCL_PIN 22

bool status_bme;

bool debug = true;             // display log message if True
bool result;

// create objects
Adafruit_BME280 bme;
WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);     
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  setup_wifi();
   
  client.setServer(mqtt_server, 1883);    // Configure MQTT connection, change port if needed.

  if (!client.connected()) {
    reconnect();
  }
  status_bme = bme.begin(0x76);  
  if (!status_bme) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }
  
  // Read temperature in Celcius
  float t = bme.readTemperature();
  // Read humidity
  float h = bme.readHumidity();
  

  // Nothing to send. Warn on MQTT debug_topic and then go to sleep for longer period.
  if ( isnan(t) || isnan(h)) {
    Serial.println("[ERROR] Please check the BME280 sensor !");
    client.publish(debug_topic, "[ERROR] Please check the BME280 sensor !", true);      // Publish humidity on broker/humid1
    esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR); //go to sleep
    Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) + " Seconds");
    Serial.println("Going to sleep now because of ERROR");
    esp_deep_sleep_start();
    return;
  }
  
  if ( debug ) {
    Serial.print("Temperature : ");
    Serial.print(t);
    Serial.print(" | Humidity : ");
    Serial.println(h);
  } 
  // Publish values to MQTT topics
  client.publish(temperature_topic, String(t).c_str(), true);
  if ( debug ) {    
    Serial.println("Temperature sent to MQTT.");
  }
  delay(500); //some delay is needed for the mqtt server to accept the message
  client.publish(humidity_topic, String(h).c_str(), true);
  if ( debug ) {
    Serial.println("Humidity sent to MQTT.");
  }
  delay(3000);

  // trigger disconnection from MQTT broker
  client.disconnect(); 
  espClient.flush();
  // wait until connection is closed completely
  while( client.state() != -1){  
      delay(10);
  }
  
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR); //go to sleep
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) + " Seconds");
  Serial.println("Going to sleep as normal now.");
  esp_deep_sleep_start();
}

//Setup connection to wifi
void setup_wifi() {
  delay(20);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi is OK ");
  Serial.print("=> ESP32 new IP address is: ");
  Serial.print(WiFi.localIP());
  Serial.println("");
}

//Reconnect to wifi if connection is lost
void reconnect() {

  while (!client.connected()) {
    Serial.print("Connecting to MQTT broker ...");
    if (client.connect(mqtt_client, mqtt_user, mqtt_password)) {
      Serial.println("OK");
    } else {
      Serial.print("[Error] Not connected: ");
      Serial.print(client.state());
      Serial.println("Wait 5 seconds before retry.");
      delay(5000);
    }
  }
}

void loop() { 
}
