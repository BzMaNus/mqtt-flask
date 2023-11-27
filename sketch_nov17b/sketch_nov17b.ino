#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <NTPClient.h>
#include <TimeLib.h>
#include <WiFiUdp.h>

const char* ssid = "Chatree01";
const char* password = "0620565502";
const char* mqttServer = "192.168.1.14";
const int led = D6;

WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(D4, DHT11);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

String getFormattedDateTime() {
  time_t now = timeClient.getEpochTime();
  struct tm* timeinfo = localtime(&now);

  char dateTime[20];
  strftime(dateTime, sizeof(dateTime), "%d %b %Y %H:%M:%S", timeinfo);

  return String(dateTime);
}

void init_wifi(const char* ssid, const char* password) {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print(("Attemping MQTT connection..."));
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.subscribe("LED");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println("try again in 5 seconds");
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message received on topic: ");
  Serial.println(topic);

  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }


  if (message.equals("on")) {
    digitalWrite(led, HIGH);
  } else if (message.equals("off")) {
    digitalWrite(led, LOW);
  }
}

void setup() {
  Serial.begin(115200);
  init_wifi(ssid, password);
  client.setServer(mqttServer, 1883);
  dht.begin();
  timeClient.begin();
  timeClient.setTimeOffset(7 * 3600);
  pinMode(led, OUTPUT);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  static unsigned long lastMillis = 0;
  if (millis() - lastMillis > 10000) {
    timeClient.update();
    String dateTime = getFormattedDateTime();

    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature();

    String data = "{\"datetime\":\"" + String(dateTime) + "\",\"humidity\":" + String(humidity, 2) + ",\"temperature\":" + String(temperature, 2) + "}";

    client.publish("dht11", data.c_str());

    lastMillis = millis();
  }
}
