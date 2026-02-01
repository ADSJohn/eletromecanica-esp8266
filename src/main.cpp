#include <Arduino.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define MMA8452_ADDR 0x1C
#define HMC5883_ADDR 0x1E
#define ONE_WIRE_BUS D4

const char *ssid = "Espaco_Vitoria";
const char *password = "espacovitoria";
const char *mqtt_server = "broker.hivemq.com";

WiFiClient espClient;
PubSubClient client(espClient);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature tempSensor(&oneWire);

void writeReg(byte addr, byte reg, byte val)
{
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

void setup()
{
  Serial.begin(115200);
  Wire.begin(D2, D1);
  tempSensor.begin();

  writeReg(MMA8452_ADDR, 0x2A, 0x00);
  writeReg(MMA8452_ADDR, 0x0E, 0x00);
  writeReg(MMA8452_ADDR, 0x2A, 0x01);

  writeReg(HMC5883_ADDR, 0x00, 0x70);
  writeReg(HMC5883_ADDR, 0x01, 0x20);
  writeReg(HMC5883_ADDR, 0x02, 0x00);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
    delay(500);

  client.setServer(mqtt_server, 1883);
}

void loop()
{
  if (!client.connected())
    client.connect("ESP_MOTOR");

  tempSensor.requestTemperatures();
  float temp = tempSensor.getTempCByIndex(0);

  Wire.beginTransmission(MMA8452_ADDR);
  Wire.write(0x01);
  Wire.endTransmission(false);
  Wire.requestFrom(MMA8452_ADDR, 6);

  int az = (Wire.read() << 8 | Wire.read()) >> 4;
  Wire.read();
  Wire.read();
  Wire.read();
  Wire.read();

  Wire.beginTransmission(HMC5883_ADDR);
  Wire.write(0x03);
  Wire.endTransmission();
  Wire.requestFrom(HMC5883_ADDR, 6);

  int mx = Wire.read() << 8 | Wire.read();
  Wire.read();
  Wire.read();
  Wire.read();
  Wire.read();

  char payload[256];
  snprintf(payload, sizeof(payload),
           "{\"temp\":%.2f,\"az\":%d,\"mx\":%d}",
           temp, az, mx);

  client.publish("eletromecanica/motor/raw", payload);
  delay(5); // 200Hz
}
