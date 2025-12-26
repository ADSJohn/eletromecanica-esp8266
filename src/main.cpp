#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

const char *rede_wifi = "nome da sua rede wifi;
    const char *senha = "senha do wifi";

const char *mosquitto_broker = "IP aonde esta rodando o mosquitto;
    const int mosquitto_port = 1883;
const char *topico = "temperatura/topic";
const char *clientId = "ESP8266_Temp_01";

WiFiClient esp8266Cliente;
PubSubClient cliente(esp8266Cliente);

void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Mensagem recebida [");
  Serial.print(topic);
  Serial.print("]: ");

  for (unsigned int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

void conectarWiFi()
{
  Serial.print("Conectando ao WiFi");
  WiFi.begin(rede_wifi, senha);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi conectado!");
  Serial.print("IP do ESP: ");
  Serial.println(WiFi.localIP());
}

void conectarMQTT()
{
  while (!cliente.connected())
  {
    Serial.print("Conectando ao Mosquitto... ");

    if (cliente.connect(clientId))
    {
      Serial.println("conectado!");
      cliente.subscribe(topico);
      cliente.publish(topico, "ESP8266 conectado com sucesso!");
    }
    else
    {
      Serial.print("falhou, rc=");
      Serial.print(cliente.state());
      Serial.println(" tentando novamente em 5 segundos");
      delay(5000);
    }
  }
}

void setup()
{
  Serial.begin(9600);
  delay(2000);

  Serial.println("\nIniciando ESP8266...");

  conectarWiFi();

  cliente.setServer(mosquitto_broker, mosquitto_port);
  cliente.setCallback(callback);

  conectarMQTT();
}

void loop()
{
  if (!cliente.connected())
  {
    conectarMQTT();
  }

  cliente.loop();

  static unsigned long ultimoEnvio = 0;
  if (millis() - ultimoEnvio > 10000)
  {
    ultimoEnvio = millis();
    cliente.publish(topico, "Temperatura: 25C");
  }
}
