![System](https://img.shields.io/badge/System-Access%20Gate%20Control-darkblue)
![Protocol](https://img.shields.io/badge/Protocol-HTTP%20Polling-yellow)
![Mode](https://img.shields.io/badge/Mode-Offline%20First-success)
![Deployment](https://img.shields.io/badge/Deployment-On--Premise-informational)
![Controller](https://img.shields.io/badge/Controller-ESP32%20%7C%20Arduino-orange)



# IoT Gate Control System (HTTP Polling)

A simple, stable, and offline-capable **IoT Gate Control System** using Arduino / ESP microcontrollers with **HTTP polling**, a **PHP backend**, and **PostgreSQL** database.

This project is designed as a **v1 implementation** that prioritizes simplicity, debuggability, and reliability on local networks (LAN), with a clear upgrade path to real-time protocols such as MQTT.

---

## 🎯 Project Goals

- Control a physical gate using microcontrollers and relay
- Validate access based on scan logic (e.g. 2 valid scans required)
- Operate reliably on a **local / offline network**
- Keep the system simple and easy to debug
- Provide a clear system architecture for future upgrades

---

## 🚀 Features

- ✅ HTTP polling-based gate control
- ✅ Supports ESP32, ESP8266, and Arduino + LAN (W5500)
- ✅ Offline-first (no internet required)
- ✅ PHP backend (plain PHP, no framework)
- ✅ PostgreSQL database
- ✅ Scan validation logic on server side
- ✅ Relay control for physical gate
- ✅ Gate status display for TV / monitor (OPEN / CLOSE)

---

## 🏗️ System Architecture

### High-Level Architecture

```mermaid
graph TD
    A[User / Operator] -->|Scan QR Code| B[Web Application]
    B -->|HTTP POST| C[PHP Backend]
    C --> D[(PostgreSQL Database)]
    E[ESP32 / Arduino] -->|HTTP Polling| C
    E -->|Relay Control| F[Gate Motor]
    C --> G[Gate Status Display<br/>(TV / Monitor)]
```

---

## 🧠 How It Works

1. User performs a **scan** via web application
2. PHP validates and updates scan count in the database
3. Microcontroller polls `gate_status.php` at a fixed interval
4. If scan conditions are met:
   - Gate status becomes `OPEN`
   - Relay is activated
5. Gate status is reset after execution
6. Status display page reflects gate state automatically

---
flowchart LR
    User[User / Operator] -->|Scan QR| Web[Web App<br/>PHP]
    Web -->|Update Scan| DB[(PostgreSQL)]
    MCU[ESP32 / Arduino] -->|HTTP Polling| Web
    MCU -->|Relay Control| Gate[Gate Motor]
    Web --> Display[Gate Status Display<br/>TV / Monitor]


---
## 🗄 Database Schema (PostgreSQL)

```sql
CREATE TABLE gate_scan (
  id SERIAL PRIMARY KEY,
  gate_id VARCHAR(10) NOT NULL,
  scan_count INT DEFAULT 0,
  updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
```
---
📡 Polling Strategy

Polling interval: 1–3 seconds

Endpoint: gate_status.php

Response format: JSON

Gate logic handled on backend to avoid duplicate triggers

Why Polling?

Simple to implement

Easy to debug

No broker or persistent connection required

Suitable for small to medium installations

---
📺 Gate Status Display

This project includes a fullscreen status display intended for TV or monitor usage.

🟥 Red background → Gate CLOSED

🟩 Green background → Gate OPEN

Automatically updates based on backend state

Suitable for:

Security post

Factory gate

Control room display

---
🧩 Supported Hardware

ESP32 (WiFi)

ESP8266 (WiFi)

Arduino UNO / MEGA + W5500 Ethernet module

Relay module (Active LOW recommended)

---
🔄 Future Improvements

Replace HTTP polling with MQTT

Support multiple gates

Authentication and access roles

Gate open timeout logic

Centralized logging and audit trail

Mobile-friendly dashboard

🧪 Design Notes

This system intentionally uses HTTP polling as a first-step architecture.
It is suitable for:

Proof of concept (PoC)

Internal gate systems

Offline LAN environments

For larger or real-time critical systems, migrating to MQTT over LAN is recommended.

📜 License

MIT License
Free to use, modify, and distribute.

👷 Author

Built as a practical IoT access control project with a focus on reliability, clarity, and future scalability.






