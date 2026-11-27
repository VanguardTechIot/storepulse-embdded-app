/**
 * @file main.cpp
 * @brief StorePulse common area node built with the Modest IoT Nano Framework.
 *
 * The node watches a common area of the gallery with a PIR sensor. When motion
 * is detected it turns on the flash, captures an image, stores it on the microSD
 * card and reports the motion to the Edge API.
 */

#include <Arduino.h>
#include <ModestIoT.h>
#include <deque>
#include <memory>
#include <time.h>
#include "Config.h"
#include "Secrets.h"
#include "CameraDriver.h"
#include "ExternalStorageDriver.h"
#include "EdgeApiGatewayClient.h"
#include "CommonAreaTelemetryPackage.h"

/**
 * @brief Common area node of a StorePulse gallery.
 *
 * Acts as the mediator between the PIR sensor, the status LED, the camera, the
 * microSD card and the Edge API: motion events coming from the sensor trigger an
 * image capture and a MOTION report.
 */
class CommonAreaNode : public Device {
private:
    WiFiConnectivityDriver& wifiDriver;
    EdgeApiGatewayClient& edgeGateway;
    PassiveInfraredSensor motionSensor;
    Led statusLed;
    CameraDriver camera;
    ExternalStorageDriver storage;
    unsigned int nextCaptureIndex = 1;

    mutable std::deque<CommonAreaTelemetryPackage> undeliveredReports;
    mutable unsigned long lastDeliveryAttemptAtInMilliseconds = 0;

    static time_t currentUtcTime() {
        const time_t now = time(nullptr);
        return now >= MINIMUM_VALID_UTC_TIME ? now : 0;
    }

    void initializePeripherals() {
        camera.initialize();
    }

    void findNextCaptureIndex() {
        char path[CAPTURE_FILENAME_BUFFER_SIZE];
        while (nextCaptureIndex < MAX_CAPTURE_INDEX) {
            snprintf(path, sizeof(path), CAPTURE_FILENAME_FORMAT, nextCaptureIndex);
            if (!storage.fileExists(path)) {
                return;
            }
            nextCaptureIndex++;
        }
    }

    /**
     * @brief Captures an image and stores it on the microSD card.
     *
     * The microSD card is mounted on the first motion event: the PIR output shares
     * GPIO 13 (SD DATA3) and keeps it low while idle, which prevents the card from
     * starting in SD mode at boot.
     */
    void captureAndStoreImage() {
        if (!storage.isInitialized()) {
            if (storage.initialize(SD_MOUNT_PATH, SD_ONE_LINE_MODE, SD_FORMAT_IF_MOUNT_FAILS)) {
                findNextCaptureIndex();
            }
        }

        if (!camera.isInitialized() || !storage.isInitialized()) {
            Serial.println(F("[CommonAreaNode] Camera or microSD not ready, image skipped"));
            return;
        }

        uint8_t* jpegBuffer = nullptr;
        size_t jpegLength = 0;
        if (!camera.captureJpeg(&jpegBuffer, &jpegLength)) {
            return;
        }

        char path[CAPTURE_FILENAME_BUFFER_SIZE];
        snprintf(path, sizeof(path), CAPTURE_FILENAME_FORMAT, nextCaptureIndex);
        if (storage.writeBinaryFile(path, jpegBuffer, jpegLength)) {
            nextCaptureIndex++;
        }
        camera.releaseJpeg(jpegBuffer);
    }

    /**
     * @brief Queues a MOTION report for the Edge API.
     * @param motionDetected true when motion starts, false when it stops.
     */
    void reportMotion(const bool motionDetected) {
        auto report = std::make_unique<CommonAreaTelemetryPackage>(DEVICE_ID, currentUtcTime(), motionDetected);
        if (!enqueueTelemetryPayload(std::move(report))) {
            Serial.println(F("[CommonAreaNode] Motion report lost, telemetry queue unavailable"));
        }
    }

    bool deliver(CommonAreaTelemetryPackage& report) const {
        lastDeliveryAttemptAtInMilliseconds = millis();

        if (edgeGateway.sendTelemetryRecord(report)) {
            return true;
        }
        if (edgeGateway.wasLastPayloadRejected()) {
            Serial.println(F("[CommonAreaNode] The Edge API rejected the report content; report dropped"));
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

    void keepUndelivered(const CommonAreaTelemetryPackage& report) const {
        if (undeliveredReports.size() >= MAXIMUM_UNDELIVERED_REPORTS) {
            undeliveredReports.pop_front();
        }
        undeliveredReports.push_back(report);
        Serial.printf("[CommonAreaNode] Report not delivered (%d); %u kept to retry\n",
                      edgeGateway.getLastResponseStatusCode(), static_cast<unsigned>(undeliveredReports.size()));
    }

public:
    CommonAreaNode(WiFiConnectivityDriver& wifiDriver, EdgeApiGatewayClient& edgeGateway)
        : Device(SAMPLING_INTERVAL_IN_MILLISECONDS, HARDWARE_TIMER_INDEX),
          wifiDriver(wifiDriver),
          edgeGateway(edgeGateway),
          motionSensor(MOTION_SENSOR_PIN, this),
          statusLed(STATUS_LED_PIN, STATUS_LED_INITIAL_STATE, this)
    {
        setTelemetryGateway(&edgeGateway);
        initializePeripherals();

        uint32_t motionTriggerHandle = appendSensorToTriggeringScheduler(
            &motionSensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, CHANGE);
        if (motionTriggerHandle == 0) {
            Serial.println(F("[CommonAreaNode] Motion sensor registration failed"));
        }

        EngineConfiguration engineConfiguration;
        engineConfiguration.samplerStackBytes = SAMPLER_STACK_BYTES;
        engineConfiguration.networkStackBytes = NETWORK_STACK_BYTES;
        engineConfiguration.queueLength = ASYNC_ENGINE_QUEUE_DEPTH;
        initializeAsynchronousEngine(engineConfiguration);
    }

    /**
     * @brief Reacts to the motion events of the PIR sensor.
     * @param event Event emitted by a sensor of this node.
     */
    void on(Event event) override {
        switch (event.identifier) {
            case PassiveInfraredSensor::MOTION_DETECTED_EVENT_IDENTIFIER:
                Serial.println(F("[CommonAreaNode] Motion detected"));
                statusLed.handle(Led::TURN_ON_COMMAND);
                captureAndStoreImage();
                reportMotion(true);
                break;
            case PassiveInfraredSensor::MOTION_STOPPED_EVENT_IDENTIFIER:
                Serial.println(F("[CommonAreaNode] Motion stopped"));
                statusLed.handle(Led::TURN_OFF_COMMAND);
                reportMotion(false);
                break;
            default:
                break;
        }
    }

protected:
    void processQueuedTelemetryData(const TelemetryPackage* rawQueueItemPayload) const override {
        CommonAreaTelemetryPackage report(*static_cast<const CommonAreaTelemetryPackage*>(rawQueueItemPayload));

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
static CommonAreaNode* commonAreaNode = nullptr;

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    Serial.println(F("[CommonAreaNode] Starting"));
    Device::setFirmwareVersion(FIRMWARE_VERSION);

    wifiDriver = new WiFiConnectivityDriver(WIFI_SSID, WIFI_PASSWORD);
    if (wifiDriver->waitForConnection(WIFI_CONNECTION_TIMEOUT_IN_MILLISECONDS)) {
        Serial.printf("[CommonAreaNode] WiFi connected, IP %s\n", wifiDriver->getLocalIpAddress().toString().c_str());
    } else {
        Serial.println(F("[CommonAreaNode] WiFi not connected yet; it keeps retrying in the background"));
    }
    TimeSynchronizationDriver::initialize(TIME_ZONE, NTP_SERVER);

    edgeGateway = new EdgeApiGatewayClient(*wifiDriver, EDGE_BASE_URL, DEVICE_ID, API_KEY,
                                           EDGE_REQUEST_TIMEOUT_IN_MILLISECONDS);
    if (!edgeGateway->authenticate()) {
        Serial.println(F("[CommonAreaNode] The Edge API did not accept the node: check DEVICE_ID, API_KEY and EDGE_BASE_URL"));
    }

    commonAreaNode = new CommonAreaNode(*wifiDriver, *edgeGateway);
    Serial.println(F("[CommonAreaNode] Ready"));
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_DELAY_IN_MILLISECONDS));
}