#include <Arduino.h>
#include <ModestIoT.h>
#include <atomic>
#include <deque>
#include <memory>
#include <time.h>
#include "Config.h"
#include "Secrets.h"
#include "EdgeApiGatewayClient.h"
#include "SmokeSensor.h"
#include "SmokeTelemetryPackage.h"

class SmokeNode : public Device {
private:
    static constexpr int NODE_STATUS_CHECK_EVENT_IDENTIFIER = 62;
    static constexpr time_t MINIMUM_VALID_UTC_TIME = 1700000000;

    WiFiConnectivityDriver& wifiDriver;
    EdgeApiGatewayClient& edgeGateway;
    SmokeSensor smokeSensor;
    Led alertLed;

    unsigned long lastReportAtInMilliseconds = 0;
    unsigned long lastWarmUpLogAtInMilliseconds = 0;
    bool alertLedActive = false;

    mutable std::deque<SmokeTelemetryPackage> undeliveredReports;
    mutable unsigned long lastDeliveryAttemptAtInMilliseconds = 0;

    mutable std::atomic<bool> edgeAlertActive{false};

    static time_t currentUtcTime() {
        const time_t now = time(nullptr);
        return now >= MINIMUM_VALID_UTC_TIME ? now : 0;
    }

    void reportSmoke(const char* reason) {
        const float concentration = smokeSensor.getSmokeConcentrationInPpm();
        auto report = std::make_unique<SmokeTelemetryPackage>(DEVICE_ID, currentUtcTime(), concentration);

        if (enqueueTelemetryPayload(std::move(report))) {
            Serial.printf("[SmokeNode] Smoke %.1f ppm, report queued (%s)\n", concentration, reason);
        } else {
            Serial.printf("[SmokeNode] Smoke %.1f ppm, report lost, telemetry queue unavailable (%s)\n",
                          concentration, reason);
        }
        lastReportAtInMilliseconds = millis();
    }

    void logWarmUp() {
        if (millis() - lastWarmUpLogAtInMilliseconds < WARM_UP_LOG_INTERVAL_IN_MILLISECONDS) {
            return;
        }
        lastWarmUpLogAtInMilliseconds = millis();
        Serial.printf("[SmokeNode] Warming up the MQ-2, %lu s left\n",
                      (smokeSensor.getRemainingWarmUpInMilliseconds() + 999) / 1000);
    }

    void refreshAlertLed() {
        const bool active = smokeSensor.isAlarmActive() || edgeAlertActive.load();
        if (active == alertLedActive) {
            return;
        }
        alertLedActive = active;
        alertLed.handle(active ? Led::TURN_ON_COMMAND : Led::TURN_OFF_COMMAND);
    }

    void onStatusCheck() {
        if (smokeSensor.isWarmingUp()) {
            logWarmUp();
        } else if (millis() - lastReportAtInMilliseconds >= REPORT_INTERVAL_IN_MILLISECONDS) {
            reportSmoke("periodic");
        }
        refreshAlertLed();
    }

    bool deliver(SmokeTelemetryPackage& report) const {
        lastDeliveryAttemptAtInMilliseconds = millis();

        if (edgeGateway.sendTelemetryRecord(report)) {
            edgeAlertActive.store(edgeGateway.wasLastTelemetryEvaluatedAsAlert());
            return true;
        }
        if (edgeGateway.wasLastPayloadRejected()) {
            Serial.println(F("[SmokeNode] The Edge API rejected the report content; report dropped"));
            return true;
        }
        return false;
    }

    void deliverUndeliveredReports() const {
        while (!undeliveredReports.empty()) {
            if (!deliver(undeliveredReports.front())) {
                return;
            }
            undeliveredReports.pop_front();
        }
    }

    void keepUndelivered(const SmokeTelemetryPackage& report) const {
        if (undeliveredReports.size() >= MAXIMUM_UNDELIVERED_REPORTS) {
            undeliveredReports.pop_front();
        }
        undeliveredReports.push_back(report);
        Serial.printf("[SmokeNode] Report not delivered (%d); %u kept to retry\n",
                      edgeGateway.getLastResponseStatusCode(), static_cast<unsigned>(undeliveredReports.size()));
    }

public:
    SmokeNode(WiFiConnectivityDriver& wifiDriver, EdgeApiGatewayClient& edgeGateway)
        : Device(SAMPLING_INTERVAL_IN_MILLISECONDS, HARDWARE_TIMER_INDEX),
          wifiDriver(wifiDriver),
          edgeGateway(edgeGateway),
          smokeSensor(SMOKE_SENSOR_PIN, SENSOR_WARM_UP_DURATION_IN_MILLISECONDS,
                      SMOKE_ALERT_THRESHOLD_IN_PPM, SMOKE_CLEAR_THRESHOLD_IN_PPM,
                      MINIMUM_ALARM_DURATION_IN_MILLISECONDS, SMOKE_PPM_PER_ADC_COUNT, this),
          alertLed(ALERT_LED_PIN, false, this)
    {
        setTelemetryGateway(&edgeGateway);

        if (!appendSensorToScheduler(&smokeSensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER)) {
            Serial.println(F("[SmokeNode] Smoke sensor registration failed"));
        }
        if (!appendSensorToScheduler(this, NODE_STATUS_CHECK_EVENT_IDENTIFIER, STATUS_CHECK_INTERVAL_IN_MILLISECONDS)) {
            Serial.println(F("[SmokeNode] Status check registration failed"));
        }

        EngineConfiguration engineConfiguration;
        engineConfiguration.samplerStackBytes = SAMPLER_STACK_BYTES;
        engineConfiguration.networkStackBytes = NETWORK_STACK_BYTES;
        engineConfiguration.queueLength = ASYNC_ENGINE_QUEUE_DEPTH;
        initializeAsynchronousEngine(engineConfiguration);
    }

    void on(Event event) override {
        switch (event.identifier) {
            case SmokeSensor::SENSOR_READY_EVENT_IDENTIFIER:
                Serial.println(F("[SmokeNode] MQ-2 ready"));
                reportSmoke("first reading");
                refreshAlertLed();
                break;
            case SmokeSensor::SMOKE_ALARM_CHANGED_EVENT_IDENTIFIER:
                Serial.printf("[SmokeNode] Smoke alarm %s\n", smokeSensor.isAlarmActive() ? "on" : "off");
                reportSmoke("alarm changed");
                refreshAlertLed();
                break;
            case NODE_STATUS_CHECK_EVENT_IDENTIFIER:
                onStatusCheck();
                break;
            default:
                break;
        }
    }

protected:
    void processQueuedTelemetryData(const TelemetryPackage* rawQueueItemPayload) const override {
        SmokeTelemetryPackage report(*static_cast<const SmokeTelemetryPackage*>(rawQueueItemPayload));

        deliverUndeliveredReports();
        report.setBufferedEvents(static_cast<int>(undeliveredReports.size()));
        if (!undeliveredReports.empty() || !deliver(report)) {
            keepUndelivered(report);
        }
    }

    void maintainNetworkServices() const override {
        if (!wifiDriver.isConnected()) {
            wifiDriver.connect();
            return;
        }
        if (!undeliveredReports.empty() &&
            millis() - lastDeliveryAttemptAtInMilliseconds >= REPORT_RETRY_INTERVAL_IN_MILLISECONDS) {
            deliverUndeliveredReports();
        }
    }
};

static WiFiConnectivityDriver* wifiDriver = nullptr;
static EdgeApiGatewayClient* edgeGateway = nullptr;
static SmokeNode* smokeNode = nullptr;

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    Serial.println(F("[SmokeNode] Starting"));
    Device::setFirmwareVersion(FIRMWARE_VERSION);

    wifiDriver = new WiFiConnectivityDriver(WIFI_SSID, WIFI_PASSWORD);
    if (wifiDriver->waitForConnection(WIFI_CONNECTION_TIMEOUT_IN_MILLISECONDS)) {
        Serial.printf("[SmokeNode] WiFi connected, IP %s\n", wifiDriver->getLocalIpAddress().toString().c_str());
    } else {
        Serial.println(F("[SmokeNode] WiFi not connected yet; it keeps retrying in the background"));
    }
    TimeSynchronizationDriver::initialize(TIME_ZONE, NTP_SERVER);

    edgeGateway = new EdgeApiGatewayClient(*wifiDriver, EDGE_BASE_URL, DEVICE_ID, API_KEY,
                                           EDGE_REQUEST_TIMEOUT_IN_MILLISECONDS);
    if (!edgeGateway->authenticate()) {
        Serial.println(F("[SmokeNode] The Edge API did not accept the node: check DEVICE_ID, API_KEY and EDGE_BASE_URL"));
    }

    smokeNode = new SmokeNode(*wifiDriver, *edgeGateway);
    Serial.println(F("[SmokeNode] Ready"));
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_DELAY_IN_MILLISECONDS));
}
