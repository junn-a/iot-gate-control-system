#include <ESP8266WiFi.h>

const char* ssid = "IDF_WirelessTAB";
const char* password = "cikupawifi123";

IPAddress local_IP(10,130,48,221);
IPAddress gateway(10,130,48,5);
IPAddress subnet(255,255,255,0);
IPAddress primaryDNS(10,126,48,1);

WiFiServer server(80);

const int relayPin = 14;

// ===== PULSE SETTINGS =====
bool relayActive = false;
unsigned long pulseStart = 0;
const unsigned long pulseDuration = 300;  // 300ms cukup untuk simulasi tombol

unsigned long lastTrigger = 0;
const unsigned long triggerDelay = 1000;  // jeda 3 detik antar scan

void setup() {

  Serial.begin(9600);

  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, HIGH);  // OFF (Active LOW)

  WiFi.config(local_IP, gateway, subnet, primaryDNS);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  Serial.println(WiFi.localIP());
  server.begin();
}

void loop() {

  WiFiClient client = server.available();

  if (client) {

    String request = client.readStringUntil('\r');
    client.flush();

    Serial.println(request);

    if (request.indexOf("GET /open") != -1) {

      unsigned long now = millis();

      if (!relayActive && (now - lastTrigger > triggerDelay)) {

        Serial.println("PULSE BUTTON");

        digitalWrite(relayPin, LOW);   // tekan tombol
        relayActive = true;
        pulseStart = now;
        lastTrigger = now;

      } else {
        Serial.println("BLOCKED");
      }

      client.println("HTTP/1.1 200 OK");
      client.println("Connection: close");
      client.println();
    }

    client.stop();
  }

  // ===== AUTO LEPAS TOMBOL =====
  if (relayActive && (millis() - pulseStart >= pulseDuration)) {

    digitalWrite(relayPin, HIGH);  // lepas tombol
    relayActive = false;

    Serial.println("BUTTON RELEASED");
  }
}
