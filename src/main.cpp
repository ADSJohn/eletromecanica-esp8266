#include <Arduino.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>

const char *ssid = "";
const char *password = "";

const char *mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;

const char *topicSensores = "eletromecanica/motor/sensores";
const char *topicStatus = "eletromecanica/motor/status";

#define MMA8452_ADDR 0x1C
#define ONE_WIRE_BUS D4

WiFiClient espClient;
PubSubClient client(espClient);

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature tempSensor(&oneWire);

void writeRegister(byte addr, byte reg, byte value)
{
  Wire.beginTransmission(addr);
  Wire.write(reg);
  Wire.write(value);
  Wire.endTransmission();
}

void reconnectMQTT()
{
  while (!client.connected())
  {
    if (client.connect("ESP8266_MOTOR_PREDITIVA"))
    {
      client.publish(topicStatus, "ESP8266 conectado - 200Hz ativo");
    }
    else
    {
      delay(1000);
    }
  }
}

void setup()
{
  Serial.begin(115200);
  delay(2000);

  Serial.println("\nInicializando sistema preditivo...");

  Wire.begin(D2, D1);
  Wire.setClock(400000);

  tempSensor.begin();

  writeRegister(MMA8452_ADDR, 0x2A, 0x00);
  writeRegister(MMA8452_ADDR, 0x0E, 0x00);
  writeRegister(MMA8452_ADDR, 0x2A, 0x01);

  WiFi.begin(ssid, password);
  Serial.print("Conectando WiFi");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(300);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado!");

  client.setServer(mqtt_server, mqtt_port);
}

void loop()
{
  static unsigned long lastSample = 0;
  const unsigned long interval = 5;

  if (!client.connected())
    reconnectMQTT();
  client.loop();

  unsigned long now = micros();

  if (now - lastSample >= interval * 1000)
  {
    lastSample = now;

    static int tempCounter = 0;
    static float temp = 0;

    if (++tempCounter >= 200)
    {
      tempSensor.requestTemperatures();
      temp = tempSensor.getTempCByIndex(0);
      tempCounter = 0;
    }

    Wire.beginTransmission(MMA8452_ADDR);
    Wire.write(0x01);
    Wire.endTransmission(false);
    Wire.requestFrom(MMA8452_ADDR, 6);

    int16_t ax = (Wire.read() << 8 | Wire.read()) >> 4;
    int16_t ay = (Wire.read() << 8 | Wire.read()) >> 4;
    int16_t az = (Wire.read() << 8 | Wire.read()) >> 4;

    char payload[128];
    snprintf(payload, sizeof(payload),
             "{\"temp\":%.2f,\"ax\":%d,\"ay\":%d,\"az\":%d}",
             temp, ax, ay, az);

    client.publish(topicSensores, payload);
    Serial.println(payload);
  }
}
