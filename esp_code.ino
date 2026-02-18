#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

const char* ssid = "***";
const char* password = "****";
const char* url = "http://IP_SERVER_KAMU/gate_status.php";

#define RELAY_PIN D1   // GPIO5 (AMAN)

unsigned long relayTimer = 0;
bool relayActive = false;

unsigned long lastRequest = 0;
const unsigned long requestInterval = 300; // ms (polling cepat tapi ringan)

void setup() {
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // relay OFF (aktif LOW)

  Serial.begin(9600);

  WiFi.begin(ssid, password);
  Serial.print("Connecting WiFi");

  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected");
  Serial.println(WiFi.localIP());
}

void loop() {

  // =========================
  // HTTP POLLING NON-BLOCKING
  // =========================
  if (millis() - lastRequest >= requestInterval) {
    lastRequest = millis();

    if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      WiFiClient client;

      http.begin(client, url);
      http.setTimeout(1500);   // timeout cepat (1.5 detik)

      int httpCode = http.GET();

      if (httpCode == 200) {
        String payload = http.getString();
        payload.trim();

        Serial.print("Server: ");
        Serial.println(payload);

        if (payload == "OPEN" && !relayActive) {
          digitalWrite(RELAY_PIN, LOW);  // relay ON
          relayActive = true;
          relayTimer = millis();

          Serial.println("RELAY ON");
        }
      } 
      else {
        Serial.print("HTTP Error: ");
        Serial.println(httpCode);
      }

      http.end();
    }
  }

  // =========================
  // RELAY TIMER (NON BLOCKING)
  // =========================
  if (relayActive && millis() - relayTimer >= 3000) { // 3 detik
    digitalWrite(RELAY_PIN, HIGH); // relay OFF
    relayActive = false;
    Serial.println("RELAY OFF");
  }

  // loop tetap ringan
}
