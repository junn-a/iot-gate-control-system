#include <WiFi.h> // Menggunakan library WiFi untuk ESP32

const char* ssid = "IDF_WirelessTAB";
const char* password = "cikupawifi123";

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);

  // Menunggu koneksi WiFi
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("=== NETWORK INFO ===");
  Serial.print("IP Address  : ");
  Serial.println(WiFi.localIP());
  Serial.print("Gateway     : ");
  Serial.println(WiFi.gatewayIP());
  Serial.print("Subnet Mask : ");
  Serial.println(WiFi.subnetMask());
  Serial.print("DNS         : ");
  Serial.println(WiFi.dnsIP());
}

void loop() {
  // Kosong
}
