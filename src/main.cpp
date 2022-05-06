#include <Arduino.h>
#include "Adafruit_Sensor.h"
#include "DHT.h"
#include "Adafruit_AHTX0.h"
#include "Adafruit_SHT31.h"
#include <OneWire.h>
#include <DallasTemperature.h>

#include <SIM76xx.h>
#include <GSMClient.h>
#include <PubSubClient.h>

#define DHT11_PIN   19
#define DHT22_PIN   23
#define DS18B20_PIN 25

DHT dht11(DHT11_PIN, DHT11);
DHT dht22(DHT22_PIN, DHT22);
Adafruit_AHTX0 aht;
Adafruit_SHT31 sht31 = Adafruit_SHT31();
OneWire oneWire(DS18B20_PIN);
DallasTemperature sensors(&oneWire);

Adafruit_Sensor *aht_humidity, *aht_temp;

const char *server = "mqtt.netpie.io";

void callback(char* topic, byte* payload, unsigned int length) {
  // handle message arrived
}

GSMClient gsm_client;
PubSubClient client(server, 1883, callback, gsm_client);

void setup() {
  Serial.begin(115200);
  
  while(!GSM.begin()) {
    Serial.println("GSM setup fail");
    delay(2000);
  }

  // Sensor begin
  dht11.begin();
  dht22.begin(); 
  Serial.println("AHT check...");
  if (!aht.begin()) {
    Serial.println("Failed to find AHT10/AHT20 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("SHT check...");
  if (!sht31.begin(0x44)) {   // Set to 0x45 for alternate i2c addr
    Serial.println("Couldn't find SHT31");
    while (1) delay(1);
  }
  Serial.println("DS18B20 check...");
  sensors.begin();
}

void loop() {
  // DHT11
  float dht11_temp = dht11.readTemperature();
  float dht11_humi = dht11.readHumidity();

  dht11_humi = isnan(dht11_humi) ? -99: dht11_humi;
  dht11_humi = isnan(dht11_humi) ? -99: dht11_humi;

  // DHT22
  float dht22_temp = dht22.readTemperature();
  float dht22_humi = dht22.readHumidity();

  dht22_temp = isnan(dht22_temp) ? -99: dht22_temp;
  dht22_humi = isnan(dht22_humi) ? -99: dht22_humi;

  // AHT20
  sensors_event_t humidity, temp;
  aht.getEvent(&humidity, &temp);
  float aht20_temp = temp.temperature;
  float aht20_humi = humidity.relative_humidity;

  float sht30_temp = sht31.readTemperature();
  float sht30_humi = sht31.readHumidity();

  sensors.requestTemperatures(); 
  float ds18b20_temp = sensors.getTempCByIndex(0);

  Serial.printf(
    "DHT11: [%.01f *C, %.01f %%RH]"
    "\tDHT22: [%.01f *C, %.01f %%RH]"
    "\tAHT20: [%.02f *C, %.02f %%RH]"
    "\tSHT30: [%.02f *C, %.02f %%RH]"
    "\tDS18B20: [%.02f *C]"
    "\n", 
    dht11_temp, dht11_humi, 
    dht22_temp, dht22_humi,
    aht20_temp, aht20_humi,
    sht30_temp, sht30_humi,
    ds18b20_temp
  );
  if (!client.connected()) {
    Serial.println("NETPIE connecting");
    client.connect("231e09e7-d5be-490e-b094-e61109cb07c0", "LbNNUQCquVYSTFMSdwoxhzatKfjRBjS3", "*5Y!nH5s7$N-F)goRiCs*hSA4-r3CLG4");
    client.subscribe("#");
    Serial.println("NETPIE connected");
  }

  char dataShadow[1023];
  sprintf(dataShadow, "{ \"data\":{ "
    "\"place\": \"AIS 4G board\","
    "\"dht11_temp\": %.01f, \"dht11_humi\": %.01f,"
    "\"dht22_temp\": %.01f, \"dht22_humi\": %.01f,"
    "\"aht20_temp\": %.02f, \"aht20_humi\": %.02f,"
    "\"sht30_temp\": %.02f, \"sht30_humi\": %.02f,"
    "\"ds18b20_temp\": %.01f"
    "}}",
    dht11_temp, dht11_humi, 
    dht22_temp, dht22_humi,
    aht20_temp, aht20_humi,
    sht30_temp, sht30_humi,
    ds18b20_temp
  );
  Serial.printf("Shadow data: %s\n", dataShadow);
  client.publish("@shadow/data/update", dataShadow);

  delay(10000);
}