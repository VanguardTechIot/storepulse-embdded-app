/**
 * @file main.cpp
 * @brief StorePulse common area node built with the Modest IoT Nano Framework.
 *
 * The node watches a common area of the gallery with a PIR sensor. When motion
 * is detected it turns on the flash, captures an image and stores it on the
 * microSD card.
 */

#include <Arduino.h>
#include <ModestIoT.h>
#include "Config.h"
#include "CameraDriver.h"
#include "ExternalStorageDriver.h"

/**
 * @brief Common area node of a StorePulse gallery.
 *
 * Acts as the mediator between the PIR sensor, the status LED, the camera and the
 * microSD card: motion events coming from the sensor trigger an image capture.
 */
class CommonAreaNode : public Device {
private:
    PassiveInfraredSensor motionSensor;
    Led statusLed;
    CameraDriver camera;
    ExternalStorageDriver storage;
    unsigned int nextCaptureIndex = 1;

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

public:
    CommonAreaNode()
        : Device(SAMPLING_INTERVAL_IN_MILLISECONDS, HARDWARE_TIMER_INDEX),
          motionSensor(MOTION_SENSOR_PIN, this),
          statusLed(STATUS_LED_PIN, STATUS_LED_INITIAL_STATE, this)
    {
        initializePeripherals();

        uint32_t motionTriggerHandle = appendSensorToTriggeringScheduler(
            &motionSensor, Sensor::MEASURE_DATA_REQUESTED_EVENT_IDENTIFIER, CHANGE);
        if (motionTriggerHandle == 0) {
            Serial.println(F("[CommonAreaNode] Motion sensor registration failed"));
        }

        EngineConfiguration engineConfiguration;
        engineConfiguration.samplerStackBytes = SAMPLER_STACK_BYTES;
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
                break;
            case PassiveInfraredSensor::MOTION_STOPPED_EVENT_IDENTIFIER:
                Serial.println(F("[CommonAreaNode] Motion stopped"));
                statusLed.handle(Led::TURN_OFF_COMMAND);
                break;
            default:
                break;
        }
    }
};

static CommonAreaNode* commonAreaNode = nullptr;

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    Serial.println(F("[CommonAreaNode] Starting"));
    commonAreaNode = new CommonAreaNode();
    Serial.println(F("[CommonAreaNode] Ready"));
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_DELAY_IN_MILLISECONDS));
}