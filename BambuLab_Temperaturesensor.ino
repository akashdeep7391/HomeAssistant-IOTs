
#include <Wire.h>
#include <AHTxx.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_Sensor.h>
#include <ArduinoJson.h>

#define MQTT_VERSION MQTT_VERSION_3_1_1

// Wifi: SSID and password
const char* WIFI_SSID = "SSID";
const char* WIFI_PASSWORD = "PASSWORD";

// MQTT: ID, server IP, port, username and password
const PROGMEM char* MQTT_CLIENT_ID = "BambuLab_THSenor";
const PROGMEM char* MQTT_SERVER_IP = "SERVER_IP";
const PROGMEM uint16_t MQTT_SERVER_PORT = 1883;
const PROGMEM char* MQTT_USER = "USERNAME";
const PROGMEM char* MQTT_PASSWORD = "PASSWORD";

// MQTT: topic
const PROGMEM char* MQTT_SENSOR_TOPIC = "homeassistant/sensor/bambulab/thsensor";

// sleeping time
const PROGMEM uint16_t SLEEPING_TIME_IN_SECONDS = 600; // 10 minutes x 60 seconds

//With Battery
// const float analogInPin = A0; //Connect a 150K ohms resistor to the battery in + and A0 Pin of the esp

// float fVoltageMatrix[20][2] = {
//     {4.00, 100},
//     {3.98, 90},
//     {3.95, 80},
//     {3.91, 70},
//     {3.87, 60},
//     {3.85, 55},
//     {3.84, 50},
//     {3.82, 45},
//     {3.80, 40},
//     {3.79, 35},
//     {3.77, 30},
//     {3.75, 25},
//     {3.73, 20},
//     {3.71, 10},
//     {3.69, 1},
//     {0, 0}
//   };

// int i, perc;

AHTxx aht10(AHTXX_ADDRESS_X38, AHT1x_SENSOR);

// DHT dht(DHTPIN, DHTTYPE);
WiFiClient wifiClient;
PubSubClient client(wifiClient);

// function called to publish the temperature and the humidity
//void publishData(float p_temperature, float p_humidity, float p_battery) { //With Battery
void publishData(float p_temperature, float p_humidity) {

  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  // INFO: the data must be converted into a string; a problem occurs when using floats...
  root["temperature"] = (String)p_temperature;
  root["humidity"] = (String)p_humidity;
  //root["battery"] = (String)p_battery; //With Battery

  root.prettyPrintTo(Serial);
  Serial.println("");

  char data[200];
  root.printTo(data, root.measureLength() + 1);
  client.publish(MQTT_SENSOR_TOPIC, data, true);
  yield();
}

// function called when a MQTT message arrived
void callback(char* p_topic, byte* p_payload, unsigned int p_length) {
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.println("INFO: Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASSWORD)) {
      Serial.println("INFO: connected");
    } else {
      Serial.print("ERROR: failed, rc=");
      Serial.print(client.state());
      Serial.println("DEBUG: try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  // init the serial
  Serial.begin(115200);

  while (aht10.begin() != true) //for ESP-01 use aht10.begin(0, 2);
    {
      Serial.println(F("AHT10_1 no connection / fail to load calib coeff")); //(F()) save string to flash & keeps dynamic memory free
      delay(5000);
    }

  Serial.println(F("AHT10 OK"));

  // init the WiFi connection
  Serial.println();
  Serial.println();
  Serial.print("INFO: Connecting to ");
  WiFi.mode(WIFI_STA);
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("INFO: WiFi connected");
  Serial.println("INFO: IP address: ");
  Serial.println(WiFi.localIP());

  // init the MQTT connection
  client.setServer(MQTT_SERVER_IP, MQTT_SERVER_PORT);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  float h = aht10.readHumidity();
  float t = aht10.readTemperature();

  // //Battery Remaning
  // float b = analogRead(analogInPin);
  // float Vin = b * 0.00426190625;
  
  // perc = 100;
  // for(i = 15; i >= 0; i--) {
  //   if(fVoltageMatrix[i][0] >= Vin) {
  //     perc = fVoltageMatrix[i + 1][1];
  //     break;
  //   }
  // }

  delay(100);

  if (h != AHTXX_ERROR || t != AHTXX_ERROR) {
    publishData(t, h);
    
  } else {
    printStatus();
    if (aht10.softReset() == true) { 
        Serial.println(F("reset success")); //as the last chance to make it alive
    } else {
        Serial.println(F("reset failed"));
    }
  }

  Serial.println("INFO: Closing the MQTT connection");
  client.disconnect();

  Serial.println("INFO: Closing the Wifi connection");
  WiFi.disconnect();

  ESP.deepSleep(SLEEPING_TIME_IN_SECONDS * 1000000, WAKE_RF_DEFAULT);
  delay(500); // wait for deep sleep to happen
}

void printStatus() {
  switch (aht10.getStatus())
  {
    case AHTXX_NO_ERROR:
      Serial.println(F("no error"));
      break;

    case AHTXX_BUSY_ERROR:
      Serial.println(F("sensor busy, increase polling time"));
      break;

    case AHTXX_ACK_ERROR:
      Serial.println(F("sensor didn't return ACK, not connected, broken, long wires (reduce speed), bus locked by slave (increase stretch limit)"));
      break;

    case AHTXX_DATA_ERROR:
      Serial.println(F("received data smaller than expected, not connected, broken, long wires (reduce speed), bus locked by slave (increase stretch limit)"));
      break;

    case AHTXX_CRC8_ERROR:
      Serial.println(F("computed CRC8 not match received CRC8, this feature supported only by AHT2x sensors"));
      break;

    default:
      Serial.println(F("unknown status"));    
      break;
  }
}
