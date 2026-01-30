#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

const char* ssid = "***";
const char* password = "****";

const char* url = "http://IP_SERVER_KAMU/gate_status.php";

#define RELAY_PIN D4

void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // relay OFF (aktif LOW)

  Serial.begin(9600);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    WiFiClient client;

    http.begin(client, url);
    int httpCode = http.GET();

    if (httpCode == 200) {
      String payload = http.getString();
      payload.trim();

      Serial.println(payload);

      if (payload == "OPEN") {
        digitalWrite(RELAY_PIN, LOW);   // relay ON
        delay(3000);                    // gate open 3 detik
        digitalWrite(RELAY_PIN, HIGH);  // relay OFF
      }
    }
    http.end();
  }

  delay(1000); // cek tiap 1 detik
}
