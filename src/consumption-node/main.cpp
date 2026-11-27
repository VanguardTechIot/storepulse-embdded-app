/**
 * @file main.cpp
 * @brief StorePulse consumption node built with the Modest IoT Nano Framework.
 *
 * The node measures the water (YF-S201) and the electricity (PZEM-004T) consumed by a stand
 * and reports them to the Edge API. It is loop-less: the meters are read locally, and the
 * node only sends a report when a meter asks for it or when the heartbeat is due.
 *
 * @par Modest IoT Nano Framework aspects illustrated:
 * - `Device` mediator with the asynchronous engine and no work in `loop()`
 * - Meters registered in the sampling scheduler, the node itself registered for its status check
 * - Reports created in the sampler task and delivered by the network task
 * - `maintainNetworkServices()` for the WiFi reconnection and the retry of undelivered reports
 */

#include <Arduino.h>
#include <ModestIoT.h>
#include <atomic>
#include <memory>
#include <time.h>
#include "Config.h"
#include "Secrets.h"
#include "ConsumptionTelemetryPackage.h"
#include "EdgeApiGatewayClient.h"
#include "ElectricityConsumptionSensor.h"
#include "WaterConsumptionSensor.h"

/**
 * @brief Consumption node of a StorePulse stand.
 *
 * Acts as the mediator between the two meters, the status LED and the Edge API:
 * - A report request of either meter sends the pending consumption of both meters.
 * - If the node has not reported for HEARTBEAT_INTERVAL_IN_MILLISECONDS, it sends the
 *   pending consumption anyway (usually 0), so the REST API keeps it as connected.
 * - If a report is not delivered, its consumption is added to the next report, so no
 *   consumption is lost while the Edge API is unreachable.
 */
class ConsumptionNode : public Device {
private:
    /// Periodic event of the node itself: heartbeat check and status LED refresh.
    static constexpr int NODE_STATUS_CHECK_EVENT_IDENTIFIER = 81;

    /// UTC times before this value mean that the NTP time is not set yet (2023-11-14).
    static constexpr time_t MINIMUM_VALID_UTC_TIME = 1700000000;

    enum class EdgeLinkState : uint8_t {
        Unknown,     ///< No report has been sent yet.
        Reachable,   ///< The last report was delivered.
        Unreachable  ///< The last report could not be delivered.
    };

    WiFiConnectivityDriver& wifiDriver;
    EdgeApiGatewayClient& edgeGateway;
    WaterConsumptionSensor waterSensor;
    ElectricityConsumptionSensor electricitySensor;
    RgbLed statusLed;

    // Sampler task state
    bool hasReported = false;
    unsigned long lastReportAtInMilliseconds = 0;
    bool waterSensorFaulted = false;
    bool electricitySensorFaulted = false;
    const RgbColor* statusColor = nullptr;

    // Network task state (only written by the network task)
    mutable ConsumptionTelemetryPackage undeliveredConsumption;
    mutable bool hasUndeliveredConsumption = false;
    mutable unsigned long lastDeliveryAttemptAtInMilliseconds = 0;

    // Shared between the network task (writer) and the sampler task (reader)
    mutable std::atomic<EdgeLinkState> edgeLinkState{EdgeLinkState::Unknown};

    static time_t currentUtcTime() {
        const time_t now = time(nullptr);
        return now >= MINIMUM_VALID_UTC_TIME ? now : 0;
    }

    /**
     * @brief Queues a report with the pending consumption of both meters.
     * @param reason Why the report is sent, for the serial log.
     * @runs_on{sampler}
     */
    void reportPendingConsumption(const char* reason) {
        auto report = std::make_unique<ConsumptionTelemetryPackage>(DEVICE_ID, currentUtcTime());
        report->addConsumption(waterSensor.getMeasurementType(), waterSensor.takePendingConsumption());
        report->addConsumption(electricitySensor.getMeasurementType(), electricitySensor.takePendingConsumption());

        if (enqueueTelemetryPayload(std::move(report))) {
            Serial.printf("[ConsumptionNode] Report queued (%s)\n", reason);
        } else {
            Serial.printf("[ConsumptionNode] Report lost, telemetry queue unavailable (%s)\n", reason);
        }
        hasReported = true;
        lastReportAtInMilliseconds = millis();
    }

    /**
     * @brief Sends the heartbeat when the node has been silent for too long.
     * @runs_on{sampler}
     */
    void checkHeartbeat() {
        if (!hasReported || millis() - lastReportAtInMilliseconds >= HEARTBEAT_INTERVAL_IN_MILLISECONDS) {
            reportPendingConsumption("heartbeat");
        }
    }

    /**
     * @brief Shows the node status with the RGB LED (section 5.1.2 of the report).
     *
     * Purple: a meter does not answer. Orange: the Edge API is unreachable. Green: normal.
     * @runs_on{sampler}
     */
    void refreshStatusLed() {
        const RgbColor* color = &RgbLed::GREEN;
        if (waterSensorFaulted || electricitySensorFaulted) {
            color = &RgbLed::PURPLE;
        } else if (!wifiDriver.isConnected() || edgeLinkState.load() == EdgeLinkState::Unreachable) {
            color = &RgbLed::ORANGE;
        }

        if (color == statusColor) {
            return;
        }
        statusColor = color;
        statusLed.setColor(*color);
        statusLed.handle(RgbLed::SET_COLOR_COMMAND);
    }

    void setSensorFault(const int sourcePin, const bool faulted) {
        if (sourcePin == WATER_FLOW_SENSOR_PIN) {
            waterSensorFaulted = faulted;
        } else if (sourcePin == ELECTRICITY_METER_RX_PIN) {
            electricitySensorFaulted = faulted;
        }
        Serial.printf("[ConsumptionNode] Meter on GPIO %d %s\n", sourcePin, faulted ? "not answering" : "recovered");
        refreshStatusLed();
    }

    /**
     * @brief Delivers a report together with any consumption that was not delivered before.
     * @param report Report to deliver.
     * @runs_on{network}
     */
    void deliver(ConsumptionTelemetryPackage& report) const {
        if (hasUndeliveredConsumption) {
            report.addConsumptionOf(undeliveredConsumption);
            report.setBufferedEvents(1);
        }
        lastDeliveryAttemptAtInMilliseconds = millis();

        if (edgeGateway.sendTelemetryRecord(report)) {
            hasUndeliveredConsumption = false;
            edgeLinkState.store(EdgeLinkState::Reachable);
            return;
        }

        if (edgeGateway.wasLastPayloadRejected()) {
            Serial.println(F("[ConsumptionNode] The Edge API rejected the report content; report dropped"));
            hasUndeliveredConsumption = false;
            edgeLinkState.store(EdgeLinkState::Reachable);
            return;
        }

        Serial.printf("[ConsumptionNode] Report not delivered (%d); kept for the next report\n",
                      edgeGateway.getLastResponseStatusCode());
        undeliveredConsumption = report;
        hasUndeliveredConsumption = true;
        edgeLinkState.store(EdgeLinkState::Unreachable);
    }

public:
    ConsumptionNode(WiFiConnectivityDriver& wifiDriver, EdgeApiGatewayClient& edgeGateway)
        : Device(SAMPLING_INTERVAL_IN_MILLISECONDS, HARDWARE_TIMER_INDEX),
          wifiDriver(wifiDriver),
          edgeGateway(edgeGateway),
          waterSensor(WATER_FLOW_SENSOR_PIN, WATER_USE_END_TIMEOUT_IN_MILLISECONDS,
                      WATER_CONTINUOUS_FLOW_REPORT_INTERVAL_IN_MILLISECONDS, this),
          electricitySensor(Serial2, ELECTRICITY_METER_RX_PIN, ELECTRICITY_METER_TX_PIN,
                            ELECTRICITY_REPORT_THRESHOLD_IN_KILOWATT_HOURS, this),
          statusLed(STATUS_LED_RED_PIN, STATUS_LED_GREEN_PIN, STATUS_LED_BLUE_PIN, STATUS_LED_COMMON_ANODE, this),
          undeliveredConsumption(DEVICE_ID, 0)
    {
        setTelemetryGateway(&edgeGateway);
        waterSensor.setFaultThreshold(SENSOR_FAULT_THRESHOLD);
        electricitySensor.setFaultThreshold(SENSOR_FAULT_THRESHOLD);

        // Registration must happen before the engine starts
        if (!appendSensorToScheduler(&waterSensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER)) {
            Serial.println(F("[ConsumptionNode] Water meter registration failed"));
        }
        if (!appendSensorToScheduler(&electricitySensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER)) {
            Serial.println(F("[ConsumptionNode] Electricity meter registration failed"));
        }
        if (!appendSensorToScheduler(this, NODE_STATUS_CHECK_EVENT_IDENTIFIER, STATUS_CHECK_INTERVAL_IN_MILLISECONDS)) {
            Serial.println(F("[ConsumptionNode] Status check registration failed"));
        }

        EngineConfiguration engineConfiguration;
        engineConfiguration.samplerStackBytes = SAMPLER_STACK_BYTES;
        engineConfiguration.networkStackBytes = NETWORK_STACK_BYTES;
        engineConfiguration.queueLength = ASYNC_ENGINE_QUEUE_DEPTH;
        initializeAsynchronousEngine(engineConfiguration);
    }

    /**
     * @brief Reacts to the events of the meters and to the status check of the node.
     * @param event Event emitted by a meter or by the scheduler.
     * @runs_on{sampler}
     */
    void on(Event event) override {
        switch (event.identifier) {
            case ConsumptionSensor::CONSUMPTION_REPORT_REQUESTED_EVENT_IDENTIFIER:
                reportPendingConsumption(event.sourceId == WATER_FLOW_SENSOR_PIN ? "water" : "electricity");
                break;
            case NODE_STATUS_CHECK_EVENT_IDENTIFIER:
                checkHeartbeat();
                refreshStatusLed();
                break;
            case Sensor::SENSOR_FAULT_EVENT_IDENTIFIER:
                setSensorFault(event.sourceId, true);
                break;
            case Sensor::SENSOR_RECOVERED_EVENT_IDENTIFIER:
                setSensorFault(event.sourceId, false);
                break;
            default:
                break;
        }
    }

protected:
    /**
     * @brief Delivers a queued report to the Edge API.
     * @param rawQueueItemPayload Report queued by reportPendingConsumption().
     * @runs_on{network}
     */
    void processQueuedTelemetryData(const TelemetryPackage* rawQueueItemPayload) const override {
        // This node only queues ConsumptionTelemetryPackage reports
        ConsumptionTelemetryPackage report(*static_cast<const ConsumptionTelemetryPackage*>(rawQueueItemPayload));
        deliver(report);
    }

    /**
     * @brief Reconnects the WiFi and retries the consumption that was not delivered.
     * @runs_on{network}
     */
    void maintainNetworkServices() const override {
        if (!wifiDriver.isConnected()) {
            wifiDriver.connect();
            return;
        }
        if (hasUndeliveredConsumption &&
            millis() - lastDeliveryAttemptAtInMilliseconds >= REPORT_RETRY_INTERVAL_IN_MILLISECONDS) {
            ConsumptionTelemetryPackage retry(DEVICE_ID, currentUtcTime());
            deliver(retry);
        }
    }
};

static WiFiConnectivityDriver* wifiDriver = nullptr;
static EdgeApiGatewayClient* edgeGateway = nullptr;
static ConsumptionNode* consumptionNode = nullptr;

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    Serial.println(F("[ConsumptionNode] Starting"));
    Device::setFirmwareVersion(FIRMWARE_VERSION);

    wifiDriver = new WiFiConnectivityDriver(WIFI_SSID, WIFI_PASSWORD);
    if (wifiDriver->waitForConnection(WIFI_CONNECTION_TIMEOUT_IN_MILLISECONDS)) {
        Serial.printf("[ConsumptionNode] WiFi connected, IP %s\n", wifiDriver->getLocalIpAddress().toString().c_str());
    } else {
        Serial.println(F("[ConsumptionNode] WiFi not connected yet; it keeps retrying in the background"));
    }
    TimeSynchronizationDriver::initialize(TIME_ZONE, NTP_SERVER);

    edgeGateway = new EdgeApiGatewayClient(*wifiDriver, EDGE_BASE_URL, DEVICE_ID, API_KEY,
                                           EDGE_REQUEST_TIMEOUT_IN_MILLISECONDS);
    if (!edgeGateway->authenticate()) {
        Serial.println(F("[ConsumptionNode] The Edge API did not accept the node: check DEVICE_ID, API_KEY and EDGE_BASE_URL"));
    }

    consumptionNode = new ConsumptionNode(*wifiDriver, *edgeGateway);
    Serial.println(F("[ConsumptionNode] Ready"));
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_DELAY_IN_MILLISECONDS));
}
