/*
  ESP32 - Kontrol 3 Servo via Web Server (Access Point Mode)
  UI: Modern SaaS Dashboard Style
  ------------------------------------------------------------
  SSID : esp_servo
  PASS : 12345678
  IP   : 192.168.4.1 (default AP IP ESP32)

  Wiring servo (default):
  Servo 1 -> GPIO 13
  Servo 2 -> GPIO 12
  Servo 3 -> GPIO 14

  Catatan:
  - Gunakan power supply eksternal 5V untuk servo jika lebih dari 1 servo.
  - GND servo harus disambung ke GND ESP32 (common ground).

  FIX (2026-09-04):
  - Arduino IDE otomatis men-generate function prototype dengan men-scan
    seluruh file mencari pola "tipe nama(...) {". Scanner ini tidak
    mengerti raw string literal R"HTML(...)HTML", sehingga ia ikut
    membaca kode JavaScript "function control(id, action){" dsb sebagai
    kode C++, lalu mencoba membuat prototype "function control(...);"
    yang error karena "function" bukan tipe C++ valid.
  - Perbaikan: semua "function nama(){}" di JS diganti menjadi
    "const nama = () => {}" (arrow function) agar tidak lagi cocok
    dengan pola yang di-scan Arduino IDE.
*/

#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>

// ================== KONFIGURASI ==================
const char* ssid     = "esp_servo";
const char* password = "12345678";

// Pin servo (ubah sesuai wiring)
#define SERVO1_PIN 13
#define SERVO2_PIN 12
#define SERVO3_PIN 14

// Sudut posisi buka/tutup (sesuaikan kebutuhan mekanik)
#define ANGLE_OPEN   90
#define ANGLE_CLOSE  0

// ===================================================

WebServer server(80);
Servo servo1, servo2, servo3;

// Status posisi tiap servo (true = open, false = close)
bool state1 = false;
bool state2 = false;
bool state3 = false;

// ================== HALAMAN WEB (HTML/CSS/JS) ==================
String htmlPage() {
  String html = R"HTML(
<!DOCTYPE html>
<html lang="id">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>ESP32 Servo Control</title>
<style>
  :root{
    --bg:#0f1117;
    --card:#171a24;
    --card-border:#242836;
    --text:#e8e9ee;
    --muted:#8b8fa3;
    --accent:#6366f1;
    --accent2:#8b5cf6;
    --green:#22c55e;
    --red:#ef4444;
    --radius:16px;
  }
  *{box-sizing:border-box; margin:0; padding:0;}
  body{
    font-family:'Segoe UI', system-ui, -apple-system, sans-serif;
    background: radial-gradient(circle at top, #1a1d29 0%, #0f1117 60%);
    color:var(--text);
    min-height:100vh;
    padding:24px 16px 60px;
  }
  .container{max-width:480px; margin:0 auto;}
  header{
    text-align:center;
    margin-bottom:28px;
  }
  header .badge{
    display:inline-flex; align-items:center; gap:6px;
    background:rgba(99,102,241,0.15);
    color:var(--accent);
    padding:6px 14px;
    border-radius:999px;
    font-size:12px;
    font-weight:600;
    letter-spacing:.3px;
    margin-bottom:14px;
  }
  header .badge .dot{
    width:7px;height:7px;border-radius:50%;
    background:var(--green);
    box-shadow:0 0 8px var(--green);
    animation:pulse 1.6s infinite;
  }
  @keyframes pulse{
    0%,100%{opacity:1;}
    50%{opacity:.4;}
  }
  h1{
    font-size:24px;
    font-weight:700;
    letter-spacing:-.3px;
  }
  header p{
    color:var(--muted);
    font-size:13px;
    margin-top:6px;
  }
  .card{
    background:var(--card);
    border:1px solid var(--card-border);
    border-radius:var(--radius);
    padding:20px 20px 22px;
    margin-bottom:16px;
    box-shadow:0 8px 24px rgba(0,0,0,0.25);
    transition:.25s;
  }
  .card:hover{
    border-color:rgba(99,102,241,0.4);
  }
  .card-top{
    display:flex;
    justify-content:space-between;
    align-items:center;
    margin-bottom:16px;
  }
  .servo-name{
    display:flex; align-items:center; gap:10px;
  }
  .icon-box{
    width:38px;height:38px;
    border-radius:10px;
    background:linear-gradient(135deg, var(--accent), var(--accent2));
    display:flex;align-items:center;justify-content:center;
    font-size:18px;
  }
  .servo-name .titles h3{
    font-size:15px;
    font-weight:600;
  }
  .servo-name .titles span{
    font-size:11px;
    color:var(--muted);
  }
  .status-pill{
    font-size:11px;
    font-weight:700;
    padding:5px 12px;
    border-radius:999px;
    text-transform:uppercase;
    letter-spacing:.4px;
  }
  .status-open{
    background:rgba(34,197,94,0.15);
    color:var(--green);
  }
  .status-close{
    background:rgba(239,68,68,0.15);
    color:var(--red);
  }
  .btn-row{
    display:grid;
    grid-template-columns:1fr 1fr;
    gap:10px;
  }
  button{
    border:none;
    padding:13px 0;
    border-radius:12px;
    font-size:14px;
    font-weight:600;
    cursor:pointer;
    transition:.2s;
    color:#fff;
  }
  button:active{transform:scale(0.96);}
  .btn-open{
    background:linear-gradient(135deg,#22c55e,#16a34a);
  }
  .btn-open:hover{filter:brightness(1.1);}
  .btn-close{
    background:linear-gradient(135deg,#ef4444,#dc2626);
  }
  .btn-close:hover{filter:brightness(1.1);}
  .btn-disabled{
    opacity:0.35;
    cursor:not-allowed;
  }
  footer{
    text-align:center;
    color:var(--muted);
    font-size:11px;
    margin-top:24px;
  }
</style>
</head>
<body>
<div class="container">
  <header>
    <div class="badge"><span class="dot"></span> ESP32 ONLINE</div>
    <h1>Servo Control Panel</h1>
    <p>Kontrol 3 aktuator servo secara real-time</p>
  </header>

  <div id="servo-list"></div>

  <footer>ESP32 Web Server &middot; Access Point Mode &middot; 192.168.4.1</footer>
</div>

<script>
const servos = [
  {id:1, name:"Servo 1", desc:"Channel GPIO 13"},
  {id:2, name:"Servo 2", desc:"Channel GPIO 12"},
  {id:3, name:"Servo 3", desc:"Channel GPIO 14"}
];

let state = {1:false, 2:false, 3:false};

const render = () => {
  const list = document.getElementById('servo-list');
  list.innerHTML = servos.map(s => {
    const isOpen = state[s.id];
    return `
    <div class="card">
      <div class="card-top">
        <div class="servo-name">
          <div class="icon-box">⚙️</div>
          <div class="titles">
            <h3>${s.name}</h3>
            <span>${s.desc}</span>
          </div>
        </div>
        <div class="status-pill ${isOpen ? 'status-open' : 'status-close'}">
          ${isOpen ? 'Terbuka' : 'Tertutup'}
        </div>
      </div>
      <div class="btn-row">
        <button class="btn-open ${isOpen ? 'btn-disabled' : ''}" onclick="control(${s.id}, 'open')">Buka</button>
        <button class="btn-close ${!isOpen ? 'btn-disabled' : ''}" onclick="control(${s.id}, 'close')">Tutup</button>
      </div>
    </div>`;
  }).join('');
};

const control = (id, action) => {
  fetch('/servo?id=' + id + '&action=' + action)
    .then(res => res.json())
    .then(data => {
      state[id] = data.state;
      render();
    })
    .catch(e => console.error(e));
};

const fetchStatus = () => {
  fetch('/status')
    .then(res => res.json())
    .then(data => {
      state = {1:data.s1, 2:data.s2, 3:data.s3};
      render();
    })
    .catch(e => console.error(e));
};

fetchStatus();
render();
</script>
</body>
</html>
)HTML";
  return html;
}

// ================== HANDLER ==================

void handleRoot() {
  server.send(200, "text/html", htmlPage());
}

void handleServo() {
  if (!server.hasArg("id") || !server.hasArg("action")) {
    server.send(400, "application/json", "{\"error\":\"missing param\"}");
    return;
  }

  int id = server.arg("id").toInt();
  String action = server.arg("action");
  bool newState = (action == "open");

  int angle = newState ? ANGLE_OPEN : ANGLE_CLOSE;

  switch (id) {
    case 1:
      servo1.write(angle);
      state1 = newState;
      break;
    case 2:
      servo2.write(angle);
      state2 = newState;
      break;
    case 3:
      servo3.write(angle);
      state3 = newState;
      break;
    default:
      server.send(404, "application/json", "{\"error\":\"invalid id\"}");
      return;
  }

  String json = "{\"id\":" + String(id) + ",\"state\":" + String(newState ? "true" : "false") + "}";
  server.send(200, "application/json", json);
}

void handleStatus() {
  String json = "{";
  json += "\"s1\":" + String(state1 ? "true" : "false") + ",";
  json += "\"s2\":" + String(state2 ? "true" : "false") + ",";
  json += "\"s3\":" + String(state3 ? "true" : "false");
  json += "}";
  server.send(200, "application/json", json);
}

// ================== SETUP & LOOP ==================

void setup() {
  Serial.begin(115200);

  // Attach servo
  servo1.attach(SERVO1_PIN);
  servo2.attach(SERVO2_PIN);
  servo3.attach(SERVO3_PIN);

  // Set posisi awal semua servo ke posisi tutup
  servo1.write(ANGLE_CLOSE);
  servo2.write(ANGLE_CLOSE);
  servo3.write(ANGLE_CLOSE);

  // Mulai WiFi Access Point
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP started. IP address: ");
  Serial.println(IP);

  // Routing
  server.on("/", handleRoot);
  server.on("/servo", handleServo);
  server.on("/status", handleStatus);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}
