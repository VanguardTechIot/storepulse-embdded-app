# StorePulse Embedded Application

Embedded application of the StorePulse IoT nodes. Each node runs on an ESP32, reads its sensors, controls its local alarm and sends telemetry to the StorePulse Edge API, which evaluates the safety rules and synchronizes the data with the StorePulse REST API.

```
ESP32 node ── HTTP/JSON + X-API-Key ──► Edge API (Flask + SQLite) ── HTTPS ──► REST API (Spring Boot)
```

## Nodes

| Node | Folder | Sensors | Measurements |
|---|---|---|---|
| Smoke node | `src/smoke-node` | MQ-2 smoke sensor | `SMOKE` (ppm) |
| Intrusion node | `src/intrusion-node` | HC-SR501 PIR sensor, DS3231 clock, active buzzer, RGB LED | `MOTION` (1 per intrusion outside attention hours) |
| Common area node | `src/common-area-node` | HC-SR501 PIR sensor, camera, microSD | Images stored on the microSD card |
| Consumption node (water done, electricity in development) | `src/consumption-node` | YF-S201 water flow sensor, PZEM-004T energy meter | `WATER` (m³), `ELECTRICITY` (kWh) |

## Hardware and wiring

### Smoke node

| Component | Pin | Notes |
|---|---|---|
| MQ-2 `VCC` | `VIN` | 5 V from the USB connection |
| MQ-2 `GND` | `GND` | Common ground |
| MQ-2 `AO` | `D34` | Through a 10 kΩ + 20 kΩ voltage divider, so the ESP32 input never exceeds 3.3 V |
| MQ-2 `DO` | Not connected | |
| Alert LED | `D2` | On-board LED |

In Wokwi the MQ-2 is powered from `3V3`, so its output is already within the ESP32 input range and the voltage divider is not needed.

### Intrusion node

| Component | Pin | Notes |
|---|---|---|
| HC-SR501 `VCC` / `GND` | `5V` / `GND` | |
| HC-SR501 `OUT` | `GPIO 27` | Movement is confirmed after 3 consecutive readings at 10 Hz |
| Active buzzer `+` | `GPIO 26` | Driven through an NPN transistor in the physical build, since a GPIO cannot supply its current |
| DS3231 `SDA` / `SCL` | `GPIO 21` / `GPIO 22` | I2C, powered from `3V3`. Keeps UTC time without network |
| RGB LED `R` / `G` / `B` | `GPIO 25` / `GPIO 33` / `GPIO 32` | 220 Ω resistor per channel |

### Common area node

Physical node only: Wokwi does not simulate the ESP32-CAM.

| Component | Pin | Notes |
|---|---|---|
| Board | ESP32-CAM (AI Thinker) | Programmed through the ESP32-CAM-MB board |
| HC-SR501 `VCC` | `5V` | Free row of the ESP32-CAM-MB headers |
| HC-SR501 `OUT` | `IO13` | |
| HC-SR501 `GND` | `GND` | |
| Camera | Built-in connector | RHYX M21-45 module (GC2145 sensor) |
| microSD | Built-in slot | FAT32, mounted in 1-bit mode (`IO2`, `IO14`, `IO15`) |
| Flash LED | `IO4` | Status LED of the node |

## Software stack

- C++ with the Arduino framework for the ESP32
- PlatformIO, used from CLion or Visual Studio Code
- ArduinoJson
- Wokwi for the device design
- Modest IoT Nano Framework on Arduino-ESP32 3.x (pioarduino platform) for the common area node and the consumption node

## Project structure

```
storepulse-embdded-app/
├── platformio.ini
├── include/
│   ├── EdgeGatewayClient.h
│   └── Secrets.h
├── src/
│   ├── smoke-node/
│   │   ├── main.cpp
│   │   └── Config.h
│   ├── intrusion-node/
│   │   ├── main.cpp
│   │   ├── Config.h
│   │   ├── AttentionHours.h / .cpp
│   │   ├── IntrusionSensor.h / .cpp
│   │   ├── RealTimeClockDriver.h / .cpp
│   │   ├── AlarmBuzzer.h / .cpp
│   │   ├── StatusLedDriver.h / .cpp
│   │   ├── IntrusionTelemetryPackage.h / .cpp
│   │   └── EdgeApiGatewayClient.h / .cpp
│   ├── common-area-node/
│   │   ├── main.cpp
│   │   ├── Config.h
│   │   ├── CameraDriver.h / .cpp
│   │   └── ExternalStorageDriver.h / .cpp
│   └── consumption-node/
│       ├── main.cpp
│       ├── Config.h
│       ├── ConsumptionSensor.h / .cpp
│       ├── WaterConsumptionSensor.h / .cpp
│       ├── ElectricityConsumptionSensor.h / .cpp
│       ├── ConsumptionTelemetryPackage.h / .cpp
│       └── EdgeApiGatewayClient.h / .cpp
└── wokwi/
    ├── smoke-node/
    │   ├── diagram.json
    │   └── wokwi.toml
    ├── intrusion-node/
    │   ├── diagram.json
    │   └── wokwi.toml
    └── consumption-node/
        ├── diagram.json
        ├── wokwi.toml
        ├── wokwi-api.h
        └── yf-s201.chip.c / .json / .wasm
```

- `include/EdgeGatewayClient.h`: WiFi connection, node authentication, telemetry delivery and an offline buffer of 20 packets.
- `src/<node>/Config.h`: pins, thresholds and intervals of each node.
- `include/Secrets.h`: network and node credentials. It is not versioned.
- `src/consumption-node/ConsumptionSensor.h`: base class of the water and electricity meters. Each meter accumulates its consumption and asks the node for a report.
- `src/consumption-node/EdgeApiGatewayClient.h`: gateway client of the consumption node for the Edge API. It sends the `X-API-Key` header, which the framework `HttpGatewayClient` does not support.
- `src/intrusion-node/AttentionHours.h`: decides whether an instant is outside attention hours, in Lima time.
- `src/intrusion-node/IntrusionSensor.h`: PIR sensor that confirms an intrusion after consecutive readings.
- `src/intrusion-node/RealTimeClockDriver.h`: DS3231 clock that keeps UTC time without network.

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

For the intrusion node, use `"device_type":"INTRUSION_NODE"`.

## Build and upload

From CLion or Visual Studio Code with the PlatformIO plugin, select the `smoke-node` environment and use Build, Upload and Monitor. From a terminal:

```bash
pio run -e smoke-node
pio run -e smoke-node -t upload
pio device monitor
```

The serial monitor runs at 115200 baud.

For the common area node, use the `common-area-node` environment:

```bash
pio run -e common-area-node
pio run -e common-area-node -t upload
```

For the consumption node, use the `consumption-node` environment and register the node in the Edge API with `"device_type":"CONSUMPTION_NODE"`:

```bash
pio run -e consumption-node
pio run -e consumption-node -t upload
```

For the intrusion node, use the `intrusion-node` environment and register the node in the Edge API with `"device_type":"INTRUSION_NODE"`:

```bash
pio run -e intrusion-node
pio run -e intrusion-node -t upload
```

## Running in Wokwi

1. Build the `smoke-node` environment.
2. Open `wokwi/smoke-node/diagram.json` with the Wokwi plugin for CLion or Visual Studio Code and start the simulator.
3. Change the gas concentration of the MQ-2 to raise the smoke reading.

The Wokwi private gateway maps `host.wokwi.internal` to the computer that runs the Edge API.

### Intrusion node

The simulation uses its own environment, `intrusion-node-wokwi`, which builds the same firmware with the `WOKWI_SIMULATION` flag:

- The PIR warm-up is 5 seconds instead of 60, since the simulated PIR is stable at once.
- The gallery is always closed, so an intrusion can be tested at any hour. The physical node keeps attention hours from 10:00 to 22:00.

Wokwi does not include the DS3231, so the diagram uses its DS1307. Both sit at I2C address `0x68` with the same time registers, so `RTClib` reads it the same way. The buzzer has a red **ALARM** LED in parallel, because Wokwi does not play a buzzer driven with a constant level.

1. Build the `intrusion-node-wokwi` environment: `pio run -e intrusion-node-wokwi`.
2. Open `wokwi/intrusion-node/diagram.json` with the Wokwi plugin and start the simulator. If the plugin reports "Configuration file not found in project", pick `wokwi.toml` and `diagram.json` of that folder with the folder icon of the Wokwi panel.
3. Wait for `[IntrusionNode] Ready` and 5 seconds of warm-up.
4. Click the PIR sensor and press **Simulate motion**. The ALARM LED turns on for 60 seconds, the RGB LED blinks red, and the serial monitor shows the `MOTION` report sent to the Edge API.
5. Each press keeps the PIR output high for 5 seconds. A new report is sent only after the output has stayed low for 5 seconds (`[IntrusionNode] Movement stopped`).

Without the Edge API running, the report is kept and retried every 30 seconds, and the RGB LED blinks amber after the alarm ends.

### Consumption node

The consumption node is simulated only. Wokwi does not include the YF-S201, so `wokwi/consumption-node` has a custom chip (`chip-yf-s201`) that outputs 7.5 pulses per second for each L/min.

1. Build the `consumption-node` environment.
2. Open `wokwi/consumption-node/diagram.json` with the Wokwi plugin and start the simulator. If the plugin reports "Configuration file not found in project", pick `wokwi.toml` and `diagram.json` of that folder with the folder icon of the Wokwi panel.
3. Click the YF-S201 chip and move the **Flow rate (L/min)** control. The serial monitor shows the flow, and when the control goes back to 0 the node reports the water of that use.

For example, 10 L/min for 60 seconds reports `WATER` ≈ 0.010 m³.

The compiled chip (`yf-s201.chip.wasm`) is versioned, so it only has to be compiled again after changing `yf-s201.chip.c`. That needs [wasi-sdk](https://github.com/WebAssembly/wasi-sdk/releases) installed in `C:\wasi-sdk`:

```powershell
& "C:\wasi-sdk\bin\clang.exe" --target=wasm32-wasip1 --sysroot "C:\wasi-sdk\share\wasi-sysroot" -nostartfiles -O2 `
  "-Wl,--import-memory" "-Wl,--export-table" "-Wl,--no-entry" `
  -o wokwi\consumption-node\yf-s201.chip.wasm wokwi\consumption-node\yf-s201.chip.c
```

## Behavior

- The MQ-2 warms up for 60 seconds before the first reading.
- The node reports every 30 seconds and immediately when its alarm changes.
- The alarm turns on above 300 ppm, the same threshold as the smoke rule of the Edge API.
- Once on, it stays on until the reading drops below 270 ppm and at least 30 seconds have passed since the last reading above the limit.
- The alert LED turns on with the local alarm or when the Edge API answers `ALERT`.
- If the Edge API does not answer, the node keeps up to 20 packets and sends them in order once it is reachable again, reporting how many are pending in `bufferedEvents`.

### Common area node behavior

- The PIR sensor works by interrupt; the node turns on the flash while motion is present.
- On every motion event the node captures a 480x320 image, converts it to JPEG and stores it on the microSD card as `capture_NNNN.jpg`, continuing after the last stored number.
- The microSD card is mounted on the first motion event, because the PIR output shares `IO13` with the card and keeps it low while idle.
- Remove the microSD card before uploading the firmware: `IO2` is a boot pin and the card keeps it high. If the upload does not start, hold `IO0`, press `RST` and release `IO0`.
- `CameraDriver` is adapted from the course ESP32-CAM example for the GC2145 sensor, and `ExternalStorageDriver` is reused from the same example.

### Intrusion node behavior

- Movement is confirmed after 3 consecutive PIR readings at 10 Hz. A single reading does not trigger the alarm.
- Attention hours are 10:00 to 22:00 in Lima time (UTC-5). They match the window the Edge API uses for `MOTION`, so both sides agree.
- Outside attention hours, a confirmed intrusion sounds the local alarm at once, for 60 seconds, and sends one `MOTION` measurement with value `1`. Inside attention hours, nothing is sent.
- Only one event is sent per intrusion. A new event requires that the movement stops for at least 5 seconds.
- The DS3231 keeps UTC time without network and sets the system clock at boot. NTP corrects it when the network is up.
- If the clock has no valid time, the node does not sound the alarm and shows purple, because an incorrect clock could sound it during attention hours.
- LED states, by priority: red blinking (intrusion), purple (error), amber blinking (no network), blue (backup power), green (normal).

### Consumption node behavior

Design guidelines of the node, in development. The water part and the electricity part follow the same rules.

- The node is built on the Modest IoT Nano Framework and is loop-less: it only acts when a sensor event arrives. The sensors are read locally, and nothing is sent while there is no consumption.
- Each measurement is the consumption since the previous report, never the cumulative counter of the meter: `WATER` in m³ and `ELECTRICITY` in kWh. The Edge API and the REST API add up every value they receive.
- Values are never negative. A negative difference, for example after the PZEM-004T counter is reset, is discarded.
- Values are not rounded to 3 decimals. Any consumption that is not sent yet is carried over to the next report, so small loads and slow leaks are not lost.
- Every report carries the pending consumption of both meters, whichever meter asked for it.
- If the Edge API cannot be reached, the consumption of the report is added to the next one and retried every 30 seconds. The framework `TelemetryBuffer` is not used: in v1.1.2 it never returns the stored packets.
- The RGB LED is green when the node works, orange when the WiFi or the Edge API cannot be reached, and purple when a meter does not answer.
- Water: the YF-S201 is read every second (about 450 pulses per liter) and its pulses are counted by interrupt. A use ends after 3 seconds without pulses, and then the node reports. While the water keeps running, it reports every 5 minutes so that a leak is detected in time.
- Electricity: the node reports when the accumulated energy reaches 10 Wh (0.01 kWh).
- Heartbeat: if the node has not reported for 10 minutes, it sends both measurements with value 0. The REST API marks a device as disconnected after 15 minutes without reports.
- Wokwi does not include the YF-S201 or the PZEM-004T, so the simulation uses custom chips (`chip-yf-s201`) with a control to change the reading, as the course BH1750 example does.
- `WaterFlowSensor` of the framework is not used: it never counts the pulses and assumes 7 pulses per milliliter. The node defines its own water sensor class.