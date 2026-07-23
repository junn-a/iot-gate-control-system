/*
  ESP32-S3 Relay Pulse Trigger
  ----------------------------
  GET http://10.130.48.15/open  -> pulses relay (simulasi tekan tombol)

  Dirancang tangguh:
  - Pakai WebServer (bukan raw socket) -> parsing HTTP jauh lebih aman & tidak gampang hang
  - WiFi auto-reconnect non-blocking (tidak pernah delay() di loop)
  - Watchdog timer -> kalau kode macet, ESP32 auto-reset sendiri
  - Static IP dengan retry, tidak pernah stuck di while(true)
  - Semua state pakai millis(), tidak ada delay() blocking di loop()
*/

#include <WiFi.h>
#include <WebServer.h>
#include "esp_task_wdt.h"

// ===== WIFI CONFIG =====
const char* ssid     = "IDF_WirelessTAB";
const char* password = "cikupawifi123";

IPAddress local_IP (10, 130, 48, 15);
IPAddress gateway  (10, 130, 48, 5);
IPAddress subnet   (255, 255, 255, 0);
IPAddress primaryDNS(10, 126, 48, 1);

WebServer server(80);

// ===== RELAY / PULSE SETTINGS =====
const int relayPin = 4;                 // sesuaikan pin ESP32-S3 yang dipakai
const unsigned long pulseDuration = 300; // ms, lama relay aktif
const unsigned long triggerDelay  = 1000; // ms, jeda minimum antar trigger (debounce)

bool relayActive = false;
unsigned long pulseStart  = 0;
unsigned long lastTrigger = 0;

// ===== WIFI RECONNECT STATE =====
unsigned long lastWifiCheck = 0;
const unsigned long wifiCheckInterval = 3000; // cek tiap 3 detik
unsigned long wifiDownSince = 0;

// ===== WATCHDOG =====
const uint32_t WDT_TIMEOUT_S = 8; // detik

// ---------------------------------------------------------
void triggerRelay() {
  unsigned long now = millis();
  if (!relayActive && (now - lastTrigger > triggerDelay)) {
    digitalWrite(relayPin, LOW);   // tekan tombol (active LOW)
    relayActive  = true;
    pulseStart   = now;
    lastTrigger  = now;
    Serial.println("[RELAY] PULSE START");
  } else {
    Serial.println("[RELAY] BLOCKED (debounce)");
  }
}

void handleOpen() {
  triggerRelay();
  server.sendHeader("Connection", "close");
  server.send(200, "text/plain", "OK");
}

void handleRoot() {
  server.sendHeader("Connection", "close");
  server.send(200, "text/plain", "ESP32-S3 Relay Controller Ready");
}

void handleNotFound() {
  server.sendHeader("Connection", "close");
  server.send(404, "text/plain", "Not Found");
}

// ---------------------------------------------------------
bool connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.config(local_IP, gateway, subnet, primaryDNS);
  WiFi.setSleep(false);              // penting: matikan WiFi power-save biar gak lag
  WiFi.begin(ssid, password);

  Serial.print("[WIFI] Connecting");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    Serial.print(".");
    delay(200); // hanya dipakai saat setup(), bukan di loop()
    esp_task_wdt_reset();
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("[WIFI] Connected, IP: ");
    Serial.println(WiFi.localIP());
    return true;
  } else {
    Serial.println("[WIFI] Connect failed, will retry in loop()");
    return false;
  }
}

// ---------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(100);

  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, HIGH); // OFF (active LOW)

  // Watchdog
  esp_task_wdt_config_t wdt_config = {
    .timeout_ms = WDT_TIMEOUT_S * 1000,
    .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,
    .trigger_panic = true
  };
  esp_task_wdt_init(&wdt_config);
  esp_task_wdt_add(NULL);

  connectWiFi();

  server.on("/", handleRoot);
  server.on("/open", handleOpen);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("[HTTP] Server started");
}

// ---------------------------------------------------------
void loop() {
  esp_task_wdt_reset(); // kasih tanda "masih hidup" ke watchdog

  server.handleClient();

  // ===== AUTO LEPAS RELAY (non-blocking) =====
  if (relayActive && (millis() - pulseStart >= pulseDuration)) {
    digitalWrite(relayPin, HIGH); // lepas tombol
    relayActive = false;
    Serial.println("[RELAY] RELEASED");
  }

  // ===== NON-BLOCKING WIFI WATCHDOG =====
  unsigned long now = millis();
  if (now - lastWifiCheck >= wifiCheckInterval) {
    lastWifiCheck = now;
    if (WiFi.status() != WL_CONNECTED) {
      if (wifiDownSince == 0) {
        wifiDownSince = now;
        Serial.println("[WIFI] Lost connection, retrying...");
      }
      WiFi.disconnect();
      WiFi.begin(ssid, password); // non-blocking reconnect attempt

      // Kalau lebih dari 30 detik gak konek-konek, restart total sebagai fallback
      if (now - wifiDownSince > 30000) {
        Serial.println("[WIFI] Down too long, restarting ESP32...");
        ESP.restart();
      }
    } else {
      wifiDownSince = 0;
    }
  }
}
