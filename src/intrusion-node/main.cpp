/**
 * @file main.cpp
 * @brief StorePulse intrusion node built with the Modest IoT Nano Framework.
 *
 * Detects movement with the PIR sensor. Outside attention hours it sounds the local
 * alarm at once and sends one MOTION event to the Edge API. Inside attention hours
 * it sends nothing.
 */

#include <Arduino.h>
#include <ModestIoT.h>
#include <deque>
#include <memory>
#include <time.h>
#include "Config.h"
#include "Secrets.h"
#include "AlarmBuzzer.h"
#include "AttentionHours.h"
#include "EdgeApiGatewayClient.h"
#include "IntrusionSensor.h"
#include "IntrusionTelemetryPackage.h"
#include "RealTimeClockDriver.h"
#include "StatusLedDriver.h"

class IntrusionNode : public Device {
private:
    static constexpr int NODE_STATUS_CHECK_EVENT_IDENTIFIER = 62;
    static constexpr int INDICATOR_REFRESH_EVENT_IDENTIFIER = 63;
    static constexpr time_t MINIMUM_VALID_UTC_TIME = 1700000000;

    WiFiConnectivityDriver& wifiDriver;
    EdgeApiGatewayClient& edgeGateway;
    RealTimeClockDriver& clockDriver;
    IntrusionSensor motionSensor;
    AlarmBuzzer alarm;
    StatusLedDriver statusLed;

    bool intrusionReported = false;
    unsigned long lastClockWriteAtInMilliseconds = 0;

    mutable std::deque<IntrusionTelemetryPackage> undeliveredReports;
    mutable unsigned long lastDeliveryAttemptAtInMilliseconds = 0;

    static time_t currentUtcTime() {
        const time_t now = time(nullptr);
        return now >= MINIMUM_VALID_UTC_TIME ? now : 0;
    }

    void reportIntrusion(const time_t recordedAt) {
        auto report = std::make_unique<IntrusionTelemetryPackage>(DEVICE_ID, recordedAt);
        if (enqueueTelemetryPayload(std::move(report))) {
            Serial.println(F("[IntrusionNode] Intrusion report queued"));
        } else {
            Serial.println(F("[IntrusionNode] Intrusion report lost, telemetry queue unavailable"));
        }
    }

    void onIntrusionConfirmed() {
        const time_t now = currentUtcTime();
        if (now == 0) {
            statusLed.setErrorFault(true);
            Serial.println(F("[IntrusionNode] Intrusion ignored: the clock has no valid time"));
            return;
        }
        if (!isOutsideAttentionHours(now)) {
            Serial.println(F("[IntrusionNode] Movement inside attention hours, no event"));
            return;
        }

        alarm.activateFor(ALARM_DURATION_IN_MILLISECONDS);
        Serial.println(F("[IntrusionNode] Intrusion outside attention hours, alarm on"));

        if (!intrusionReported) {
            intrusionReported = true;
            reportIntrusion(now);
        }
    }

    void onIntrusionEnded() {
        intrusionReported = false;
        Serial.println(F("[IntrusionNode] Movement stopped"));
    }

    void writeClockWhenNetworkIsUp() {
        const unsigned long nowInMilliseconds = millis();
        const time_t now = currentUtcTime();
        if (!wifiDriver.isConnected() || now == 0 ||
            nowInMilliseconds - lastClockWriteAtInMilliseconds < RTC_SYNC_INTERVAL_IN_MILLISECONDS) {
            return;
        }
        clockDriver.writeUtcTime(now);
        lastClockWriteAtInMilliseconds = nowInMilliseconds;
    }

    void onStatusCheck() {
        statusLed.setErrorFault(currentUtcTime() == 0);
        statusLed.setNetworkLost(!wifiDriver.isConnected() || !undeliveredReports.empty());
        statusLed.setBackupPower(false);
        writeClockWhenNetworkIsUp();
    }

    void refreshIndicators() {
        alarm.update();
        statusLed.setEventDetected(alarm.isActive());
        statusLed.refresh();
    }

    bool deliver(IntrusionTelemetryPackage& report) const {
        lastDeliveryAttemptAtInMilliseconds = millis();

        if (edgeGateway.sendTelemetryRecord(report)) {
            return true;
        }
        if (edgeGateway.wasLastPayloadRejected()) {
            Serial.println(F("[IntrusionNode] The Edge API rejected the report content; report dropped"));
            return true;
        }
        if (edgeGateway.getLastResponseStatusCode() == 401) {
            edgeGateway.authenticate();
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

    void keepUndelivered(const IntrusionTelemetryPackage& report) const {
        if (undeliveredReports.size() >= MAXIMUM_UNDELIVERED_REPORTS) {
            undeliveredReports.pop_front();
        }
        undeliveredReports.push_back(report);
        Serial.printf("[IntrusionNode] Report not delivered (%d); %u kept to retry\n",
                      edgeGateway.getLastResponseStatusCode(), static_cast<unsigned>(undeliveredReports.size()));
    }

public:
    IntrusionNode(WiFiConnectivityDriver& wifiDriver, EdgeApiGatewayClient& edgeGateway,
                  RealTimeClockDriver& clockDriver)
        : Device(SAMPLING_INTERVAL_IN_MILLISECONDS, HARDWARE_TIMER_INDEX),
          wifiDriver(wifiDriver),
          edgeGateway(edgeGateway),
          clockDriver(clockDriver),
          motionSensor(MOTION_SENSOR_PIN, PIR_WARM_UP_IN_MILLISECONDS, MOTION_CONFIRMATION_READINGS,
                       MOTION_ENDED_AFTER_IN_MILLISECONDS, this),
          alarm(ALARM_PIN, ALARM_ACTIVE_LEVEL),
          statusLed(STATUS_LED_RED_PIN, STATUS_LED_GREEN_PIN, STATUS_LED_BLUE_PIN, STATUS_LED_COMMON_ANODE)
    {
        setTelemetryGateway(&edgeGateway);

        if (!appendSensorToScheduler(&motionSensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER)) {
            Serial.println(F("[IntrusionNode] Motion sensor registration failed"));
        }
        if (!appendSensorToScheduler(this, NODE_STATUS_CHECK_EVENT_IDENTIFIER, STATUS_CHECK_INTERVAL_IN_MILLISECONDS)) {
            Serial.println(F("[IntrusionNode] Status check registration failed"));
        }
        if (!appendSensorToScheduler(this, INDICATOR_REFRESH_EVENT_IDENTIFIER, INDICATOR_REFRESH_INTERVAL_IN_MILLISECONDS)) {
            Serial.println(F("[IntrusionNode] Indicator refresh registration failed"));
        }

        EngineConfiguration engineConfiguration;
        engineConfiguration.samplerStackBytes = SAMPLER_STACK_BYTES;
        engineConfiguration.networkStackBytes = NETWORK_STACK_BYTES;
        engineConfiguration.queueLength = ASYNC_ENGINE_QUEUE_DEPTH;
        initializeAsynchronousEngine(engineConfiguration);
    }

    void on(Event event) override {
        switch (event.identifier) {
            case IntrusionSensor::INTRUSION_CONFIRMED_EVENT_IDENTIFIER:
                onIntrusionConfirmed();
                break;
            case IntrusionSensor::INTRUSION_ENDED_EVENT_IDENTIFIER:
                onIntrusionEnded();
                break;
            case NODE_STATUS_CHECK_EVENT_IDENTIFIER:
                onStatusCheck();
                break;
            case INDICATOR_REFRESH_EVENT_IDENTIFIER:
                refreshIndicators();
                break;
            default:
                break;
        }
    }

protected:
    void processQueuedTelemetryData(const TelemetryPackage* rawQueueItemPayload) const override {
        IntrusionTelemetryPackage report(*static_cast<const IntrusionTelemetryPackage*>(rawQueueItemPayload));

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
static RealTimeClockDriver* clockDriver = nullptr;
static IntrusionNode* intrusionNode = nullptr;

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    Serial.println(F("[IntrusionNode] Starting"));
    Device::setFirmwareVersion(FIRMWARE_VERSION);

    clockDriver = new RealTimeClockDriver();
    if (!clockDriver->initialize()) {
        Serial.println(F("[IntrusionNode] Clock not ready; intrusions are ignored until a valid time is known"));
    }

    wifiDriver = new WiFiConnectivityDriver(WIFI_SSID, WIFI_PASSWORD);
    if (wifiDriver->waitForConnection(WIFI_CONNECTION_TIMEOUT_IN_MILLISECONDS)) {
        Serial.printf("[IntrusionNode] WiFi connected, IP %s\n", wifiDriver->getLocalIpAddress().toString().c_str());
    } else {
        Serial.println(F("[IntrusionNode] WiFi not connected yet; it keeps retrying in the background"));
    }
    TimeSynchronizationDriver::initialize(TIME_ZONE, NTP_SERVER);

    edgeGateway = new EdgeApiGatewayClient(*wifiDriver, EDGE_BASE_URL, DEVICE_ID, API_KEY,
                                           EDGE_REQUEST_TIMEOUT_IN_MILLISECONDS);
    if (!edgeGateway->authenticate()) {
        Serial.println(F("[IntrusionNode] The Edge API did not accept the node; check DEVICE_ID, API_KEY and EDGE_BASE_URL"));
    }

    intrusionNode = new IntrusionNode(*wifiDriver, *edgeGateway, *clockDriver);
    Serial.println(F("[IntrusionNode] Ready"));
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_DELAY_IN_MILLISECONDS));
}