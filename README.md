# StorePulse Embedded Application

Embedded application of the StorePulse IoT nodes. Each node runs on an ESP32, reads its sensors, controls its local alarm and sends telemetry to the StorePulse Edge API, which evaluates the safety rules and synchronizes the data with the StorePulse REST API.

```
ESP32 node ── HTTP/JSON + X-API-Key ──► Edge API (Flask + SQLite) ── HTTPS ──► REST API (Spring Boot)
```

## Nodes

| Node | Folder | Sensors | Measurements |
|---|---|---|---|
| Smoke node | `src/smoke-node` | MQ-2 smoke sensor, optional DHT22 | `SMOKE` (ppm), `TEMPERATURE` (°C) |

## Hardware and wiring

### Smoke node

| Component | Pin | Notes |
|---|---|---|
| MQ-2 `VCC` | `VIN` | 5 V from the USB connection |
| MQ-2 `GND` | `GND` | Common ground |
| MQ-2 `AO` | `D34` | Through a 10 kΩ + 20 kΩ voltage divider, so the ESP32 input never exceeds 3.3 V |
| MQ-2 `DO` | Not connected | |
| DHT22 `DATA` | `D15` | Optional, enabled with `USE_DHT22` |
| Alert LED | `D2` | On-board LED |

## Software stack

- C++ with the Arduino framework for the ESP32
- PlatformIO, used from CLion or Visual Studio Code
- ArduinoJson and DHT sensor library for ESPx
- Wokwi for the device design

## Project structure

```
storepulse-embdded-app/
├── platformio.ini
├── include/
│   ├── EdgeGatewayClient.h
│   └── Secrets.h
├── src/
│   └── smoke-node/
│       ├── main.cpp
│       └── Config.h
└── wokwi/
    └── smoke-node/
        ├── diagram.json
        └── wokwi.toml
```

- `include/EdgeGatewayClient.h`: WiFi connection, node authentication, telemetry delivery and an offline buffer of 20 packets.
- `src/<node>/Config.h`: pins, thresholds and intervals of each node.
- `include/Secrets.h`: network and node credentials. It is not versioned.

## Configuration

Create `include/Secrets.h` with these values:

```cpp
#pragma once

const char* const WIFI_SSID = "";
const char* const WIFI_PASSWORD = "";
const char* const EDGE_BASE_URL = "";
const char* const DEVICE_ID = "";
const char* const API_KEY = "";
```

| Value | ESP32 | Wokwi |
|---|---|---|
| `WIFI_SSID` | 2.4 GHz network of the gallery | `Wokwi-GUEST` |
| `WIFI_PASSWORD` | Network password | Empty |
| `EDGE_BASE_URL` | `http://<edge computer IP>:5000/api/v1` | `http://host.wokwi.internal:5000/api/v1` |
| `DEVICE_ID` | Serial number of the IoT device in the REST API | Same |
| `API_KEY` | Key issued by the Edge API for the node | Same |

## Edge provisioning

Each node is registered once in the Edge API, which returns its API key. The request needs the access token of a gallery administrator and the `id` of the IoT device in the REST API.

```bash
curl -X POST http://127.0.0.1:5000/api/v1/iam/devices \
  -H "Authorization: Bearer <access token>" \
  -H "Content-Type: application/json" \
  -d '{"device_id":"<serial number>","device_type":"SMOKE_NODE","platform_device_id":<device id>}'
```

Copy the returned `api_key` to `API_KEY` in `include/Secrets.h`. It is only shown once.

## Build and upload

From CLion or Visual Studio Code with the PlatformIO plugin, select the `smoke-node` environment and use Build, Upload and Monitor. From a terminal:

```bash
pio run -e smoke-node
pio run -e smoke-node -t upload
pio device monitor
```

The serial monitor runs at 115200 baud.

## Running in Wokwi

1. Build the `smoke-node` environment.
2. Open `wokwi/smoke-node/diagram.json` with the Wokwi plugin for CLion or Visual Studio Code and start the simulator.
3. Change the gas concentration of the MQ-2 to raise the smoke reading.

The Wokwi private gateway maps `host.wokwi.internal` to the computer that runs the Edge API.

## Behavior

- The MQ-2 warms up for 60 seconds before the first reading.
- The node reports every 30 seconds and immediately when its alarm changes.
- The alarm turns on above 300 ppm, the same threshold as the smoke rule of the Edge API.
- Once on, it stays on until the reading drops below 270 ppm and at least 30 seconds have passed since the last reading above the limit.
- The alert LED turns on with the local alarm or when the Edge API answers `ALERT`.
- If the Edge API does not answer, the node keeps up to 20 packets and sends them in order once it is reachable again, reporting how many are pending in `bufferedEvents`.
