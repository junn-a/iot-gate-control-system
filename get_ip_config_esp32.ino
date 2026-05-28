#include <WiFi.h> // Menggunakan library WiFi khusus untuk ESP32

const char* ssid = "IDF_WirelessTAB";
const char* password = "cikupawifi123";

IPAddress local_IP(10, 130, 48, 107);
IPAddress gateway(10, 130, 48, 5);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(10, 126, 48, 1);

WiFiServer server(80);

// Pin relay sesuaikan dengan pin GPIO ESP32 yang Anda gunakan (misal: GPIO 14)
const int relayPin = 14; 

// ===== PULSE SETTINGS =====
bool relayActive = false;
unsigned long pulseStart = 0;
const unsigned long pulseDuration = 300;  // 300ms untuk simulasi tekan tombol

unsigned long lastTrigger = 0;
const unsigned long triggerDelay = 1000;  // jeda 1 detik antar scan

void setup() {
  Serial.begin(115200); // Disarankan menggunakan 115200 untuk ESP32

  pinMode(relayPin, OUTPUT);
  digitalWrite(relayPin, HIGH);  // OFF (Active LOW)

  // Konfigurasi IP Statis untuk ESP32 (Urutan: local, gateway, subnet, dns)
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS)) {
    Serial.println("STA Failed to configure");
  }
  
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected! IP Address: ");
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

        digitalWrite(relayPin, LOW);   // Tekan tombol (Aktif)
        relayActive = true;
        pulseStart = now;
        lastTrigger = now;

      } else {
        Serial.println("BLOCKED");
      }

      // Kirim respon HTTP standard agar client (browser/apps) tahu request sukses
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/plain");
      client.println("Connection: close");
      client.println();
      client.println("OK");
    }

    client.stop();
  }

  // ===== AUTO LEPAS TOMBOL (NON-BLOCKING) =====
  if (relayActive && (millis() - pulseStart >= pulseDuration)) {
    digitalWrite(relayPin, HIGH);  // Lepas tombol (Mati)
    relayActive = false;

    Serial.println("BUTTON RELEASED");
  }
}
