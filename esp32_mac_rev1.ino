/*
  ESP32-S3 Relay Pulse Trigger  (versi perbaikan stabilitas Wi-Fi)
  ----------------------------------------------------------------
  GET /open    -> pulse relay (simulasi tekan tombol)
  GET /status  -> JSON telemetri (rssi, uptime, free_heap, reset_reason, dll)
  GET /        -> JSON telemetri yang sama (dipakai dashboard sebagai ping)

  Perbaikan utama:
  - Reconnect bertahap, TIDAK lagi disconnect+begin tiap 3 detik
    (itu membatalkan proses asosiasi di sinyal lemah)
  - Deteksi "zombie": Wi-Fi terlihat connected tapi tidak ada request masuk
  - Pilih AP dengan sinyal terkuat (SSID sama di banyak AP)
  - Telemetri untuk diagnosa (reset_reason, alasan disconnect, RSSI)
*/

#include <WiFi.h>
#include <WebServer.h>
#include <esp_task_wdt.h>
#include <esp_mac.h>
#include <esp_system.h>

// ===== WIFI CONFIG =====
const char* ssid     = "IDF_WirelessTAB";
const char* password = "BeldenCAT6";

IPAddress local_IP (10, 130, 48, 35);
IPAddress gateway  (10, 130, 48, 5);
IPAddress subnet   (255, 255, 255, 0);
IPAddress primaryDNS(10, 126, 48, 1);   // CEK: beda segmen dengan 10.130.x.x, kemungkinan typo

WebServer server(80);

// ===== RELAY / PULSE SETTINGS =====
const int relayPin = 4;
const unsigned long pulseDuration = 300;   // ms
const unsigned long triggerDelay  = 1000;  // ms, debounce

bool relayActive = false;
unsigned long pulseStart  = 0;
unsigned long lastTrigger = 0;

// ===== WIFI RECOVERY (bertahap) =====
const unsigned long wifiCheckInterval = 1000;
const unsigned long STAGE1_MS = 8000;    // WiFi.reconnect()
const unsigned long STAGE2_MS = 20000;   // reset Wi-Fi total
const unsigned long STAGE3_MS = 60000;   // restart ESP

unsigned long lastWifiCheck = 0;
unsigned long wifiDownSince = 0;
uint8_t wifiStage = 0;

// ===== DETEKSI ZOMBIE (connected tapi tidak ada trafik) =====
// Dashboard polling tiap ~10 detik, jadi 2 menit tanpa request = tidak normal.
// Kalau gate ini dipakai tanpa dashboard, naikkan nilainya.
const unsigned long SILENCE_SOFT_MS = 120000;  // 2 menit -> reset Wi-Fi
const unsigned long SILENCE_HARD_MS = 300000;  // 5 menit -> restart ESP
unsigned long lastRequestAt = 0;
bool silenceSoftDone = false;

// ===== DIAGNOSA =====
volatile uint8_t  lastDiscReason = 0;
volatile uint32_t discCount = 0;
volatile bool     needSleepOff = false;

RTC_NOINIT_ATTR uint32_t rtcMagic;
RTC_NOINIT_ATTR uint32_t rtcCause;
const uint32_t RTC_MAGIC = 0xC0FFEE42;
uint32_t selfRestartCause = 0;  // 0 = bukan restart dari script, 1 = wifi_down, 2 = no_traffic

// ===== WATCHDOG =====
const uint32_t WDT_TIMEOUT_S = 8;

// ---------------------------------------------------------
void printMacAddress() {
  uint8_t mac[6];
  if (esp_read_mac(mac, ESP_MAC_WIFI_STA) == ESP_OK) {
    Serial.printf("[INFO] MAC Address ESP32-S3: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  } else {
    Serial.println("[ERROR] Gagal membaca MAC Address!");
  }
}

const char* resetReasonStr() {
  switch (esp_reset_reason()) {
    case ESP_RST_POWERON:   return "poweron";
    case ESP_RST_SW:        return "software";
    case ESP_RST_PANIC:     return "panic";
    case ESP_RST_INT_WDT:   return "int_wdt";
    case ESP_RST_TASK_WDT:  return "task_wdt";
    case ESP_RST_WDT:       return "wdt";
    case ESP_RST_BROWNOUT:  return "brownout";
    case ESP_RST_EXT:       return "external";
    case ESP_RST_DEEPSLEEP: return "deepsleep";
    default:                return "other";
  }
}

const char* selfRestartStr() {
  switch (selfRestartCause) {
    case 1:  return "wifi_down";
    case 2:  return "no_traffic";
    default: return "none";
  }
}

void safeRestart(uint32_t cause) {
  digitalWrite(relayPin, HIGH);   // pastikan relay OFF
  rtcMagic = RTC_MAGIC;
  rtcCause = cause;
  delay(100);
  ESP.restart();
}

// ---------------------------------------------------------
void onWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      lastDiscReason = info.wifi_sta_disconnected.reason;
      discCount++;
      break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      needSleepOff = true;   // diterapkan di loop(), bukan di callback
      break;
    default:
      break;
  }
}

void startWiFi() {
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.setScanMethod(WIFI_ALL_CHANNEL_SCAN);          // scan semua channel
  WiFi.setSortMethod(WIFI_CONNECT_AP_BY_SIGNAL);      // pilih AP sinyal terkuat
  WiFi.config(local_IP, gateway, subnet, primaryDNS);
  WiFi.begin(ssid, password);
  WiFi.setSleep(false);                               // matikan power-save Wi-Fi
}

void hardResetWiFi() {
  Serial.println("[WIFI] Hard reset Wi-Fi stack");
  WiFi.disconnect(true);
  delay(100);
  esp_task_wdt_reset();
  startWiFi();
  server.stop();
  server.begin();
}

void waitInitialConnect() {
  Serial.print("[WIFI] Connecting");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 20000) {
    Serial.print(".");
    delay(200);
    esp_task_wdt_reset();
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("[WIFI] Connected, IP: ");
    Serial.print(WiFi.localIP());
    Serial.printf("  RSSI: %d dBm  CH: %d\n", WiFi.RSSI(), WiFi.channel());
  } else {
    Serial.printf("[WIFI] Belum connect (disc reason %u), loop() akan lanjut recovery\n", lastDiscReason);
  }
}

// ---------------------------------------------------------
void markAlive() {
  lastRequestAt = millis();
  silenceSoftDone = false;
}

String buildStatusJson() {
  char buf[320];
  snprintf(buf, sizeof(buf),
    "{\"status\":\"ok\",\"rssi\":%d,\"uptime\":%lu,\"free_heap\":%u,"
    "\"reset_reason\":\"%s\",\"self_restart\":\"%s\",\"channel\":%d,"
    "\"disc_count\":%lu,\"last_disc_reason\":%u,\"relay_active\":%s}",
    WiFi.RSSI(),
    (unsigned long)(millis() / 1000),
    (unsigned)ESP.getFreeHeap(),
    resetReasonStr(),
    selfRestartStr(),
    WiFi.channel(),
    (unsigned long)discCount,
    (unsigned)lastDiscReason,
    relayActive ? "true" : "false");
  return String(buf);
}

void triggerRelay() {
  unsigned long now = millis();
  if (!relayActive && (now - lastTrigger > triggerDelay)) {
    digitalWrite(relayPin, LOW);   // active LOW
    relayActive = true;
    pulseStart  = now;
    lastTrigger = now;
    Serial.println("[RELAY] PULSE START");
  } else {
    Serial.println("[RELAY] BLOCKED (debounce)");
  }
}

void handleOpen() {
  markAlive();
  triggerRelay();
  server.sendHeader("Connection", "close");
  server.send(200, "text/plain", "OK");
}

void handleStatus() {
  markAlive();
  server.sendHeader("Connection", "close");
  server.send(200, "application/json", buildStatusJson());
}

void handleNotFound() {
  markAlive();
  server.sendHeader("Connection", "close");
  server.send(404, "text/plain", "Not Found");
}

// ---------------------------------------------------------
void setup() {
  Serial.begin(115200);
  delay(200);

  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, HIGH); // OFF (active LOW)

  // Cek apakah restart ini dipicu oleh script sendiri
  if (esp_reset_reason() == ESP_RST_SW && rtcMagic == RTC_MAGIC) {
    selfRestartCause = rtcCause;
  }
  rtcMagic = 0;

  printMacAddress();
  Serial.printf("[INFO] Reset reason: %s, self restart: %s\n", resetReasonStr(), selfRestartStr());

  esp_task_wdt_config_t wdt_config = {
    .timeout_ms = WDT_TIMEOUT_S * 1000,
    .idle_core_mask = (1 << portNUM_PROCESSORS) - 1,
    .trigger_panic = true
  };
  esp_task_wdt_reconfigure(&wdt_config);
  esp_task_wdt_add(NULL);

  WiFi.onEvent(onWiFiEvent);
  startWiFi();
  waitInitialConnect();

  server.on("/", handleStatus);
  server.on("/status", handleStatus);
  server.on("/open", handleOpen);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("[HTTP] Server started");

  lastRequestAt = millis();   // beri masa tenggang sebelum deteksi zombie
}

// ---------------------------------------------------------
void loop() {
  esp_task_wdt_reset();
  server.handleClient();

  unsigned long now = millis();

  // ===== AUTO LEPAS RELAY (non-blocking) =====
  if (relayActive && (now - pulseStart >= pulseDuration)) {
    digitalWrite(relayPin, HIGH);
    relayActive = false;
    Serial.println("[RELAY] RELEASED");
  }

  if (needSleepOff) {
    needSleepOff = false;
    WiFi.setSleep(false);
    Serial.printf("[WIFI] Got IP, RSSI %d dBm, CH %d\n", WiFi.RSSI(), WiFi.channel());
  }

  // ===== WIFI RECOVERY BERTAHAP =====
  if (now - lastWifiCheck >= wifiCheckInterval) {
    lastWifiCheck = now;

    if (WiFi.status() == WL_CONNECTED) {
      if (wifiDownSince != 0) {
        Serial.printf("[WIFI] Pulih setelah %lu ms\n", now - wifiDownSince);
        wifiDownSince = 0;
        wifiStage = 0;
      }

      // --- Deteksi zombie: connected tapi sepi trafik ---
      unsigned long silent = now - lastRequestAt;
      if (silent > SILENCE_HARD_MS) {
        Serial.println("[ZOMBIE] Tidak ada request >5 menit, restart ESP");
        safeRestart(2);
      } else if (silent > SILENCE_SOFT_MS && !silenceSoftDone) {
        silenceSoftDone = true;
        Serial.println("[ZOMBIE] Tidak ada request >2 menit, reset Wi-Fi");
        hardResetWiFi();
      }

    } else {
      if (wifiDownSince == 0) {
        wifiDownSince = now;
        wifiStage = 0;
        Serial.printf("[WIFI] Putus (reason %u), menunggu auto-reconnect...\n", lastDiscReason);
      }
      unsigned long down = now - wifiDownSince;

      if (down > STAGE3_MS) {
        Serial.println("[WIFI] Down >60 detik, restart ESP");
        safeRestart(1);
      } else if (down > STAGE2_MS && wifiStage < 2) {
        wifiStage = 2;
        hardResetWiFi();
      } else if (down > STAGE1_MS && wifiStage < 1) {
        wifiStage = 1;
        Serial.println("[WIFI] WiFi.reconnect()");
        WiFi.reconnect();
      }
    }
  }
}
