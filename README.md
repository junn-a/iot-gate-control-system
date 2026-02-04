![System](https://img.shields.io/badge/System-Access%20Gate%20Control-darkblue)
![Protocol](https://img.shields.io/badge/Protocol-HTTP%20Polling-yellow)
![Mode](https://img.shields.io/badge/Mode-Offline%20First-success)
![Deployment](https://img.shields.io/badge/Deployment-On--Premise-informational)
![Controller](https://img.shields.io/badge/Controller-ESP32%20%7C%20Arduino-orange)



# IoT Gate Control System (HTTP Polling)

A simple, stable, and offline-capable **IoT Gate Control System** using ESP microcontrollers with **HTTP polling**, a **PHP backend**, and **PostgreSQL** database.

This project is designed as a **v1 implementation** that prioritizes simplicity, debuggability, and reliability on networks (WIFI), with a clear upgrade path to real-time protocols such as MQTT.

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
- ✅ Supports ESP32, ESP8266, and Arduino + LAN
- ✅ Offline-first (no internet required)
- ✅ PHP backend 
- ✅ PostgreSQL database
- ✅ Scan validation logic on server side
- ✅ Relay control for physical gate

---

## 🏗️ System Architecture

### Architecture

```mermaid
graph TD
    A[User Operator] -->|Scan QR Code| B[Web App]
    B -->|Send Scan Data| C[PHP Server]
    C --> D[(PostgreSQL)]
    E[Microcontroller] -->|Poll Status| C
    E -->|Control Relay| F[Gate]
    C --> G[Gate Status Display]

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
## Flow Proccess
```mermaid
flowchart LR
    A[Pick item & scan barcode]
    B{Valid in WMS?}
    C[Update stock & print label]
    D[ESP activates relay]
    E[Gate opens → item passes]
    F[Gate closes & send to WHL]
    G[Gate remains closed]

    A --> B
    B -->|Yes| C --> D --> E --> F
    B -->|No| G

```
## Sequence Diagram (System Interaction)
```mermaid
sequenceDiagram
    participant Crew
    participant Scanner
    participant WMS
    participant ESP8266
    participant Gate

    Crew->>Scanner: Scan barcode
    Scanner->>WMS: Send barcode data
    WMS-->>Scanner: Validation result

    alt Valid
        WMS->>WMS: Update stock & print label
        WMS->>ESP8266: Send VALID status
        ESP8266->>Gate: Activate relay
        Gate-->>Crew: Gate opens
        Gate-->>Gate: Auto close
    else Invalid
        WMS->>ESP8266: Send INVALID status
        ESP8266-->>Gate: Keep gate closed
    end


```
---
## Design

> Gate Design in `/docs` folder


![Gate](docs/gate_design.jpg)
---








