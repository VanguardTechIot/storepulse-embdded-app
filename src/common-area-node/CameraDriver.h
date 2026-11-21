/**
 * @file CameraDriver.h
 * @brief Wrapper around the esp32-camera library for the ESP32-CAM image sensor.
 *
 * Based on the CameraDriver of the course ESP32-CAM example. Adapted for the
 * GC2145 sensor (RHYX M21-45 module), which has no hardware JPEG encoder:
 * frames are captured in RGB565 and converted to JPEG by software.
 *
 * @note This class is sketch-level only. It wraps the esp32-camera ESP-IDF component
 *       and is not part of the Modest IoT Nano Framework.
 */

#ifndef CameraDriver_h
#define CameraDriver_h

#include <esp_camera.h>

/**
 * @brief Wrapper around the esp32-camera library for image capture.
 *
 * Follows the Driver pattern of the framework: a low-level abstraction used
 * directly by the Device subclass. It is not a Sensor because a JPEG image
 * does not fit in the Event/TelemetryPackage pipeline.
 */
class CameraDriver {
public:
    /**
     * @brief Constructs a CameraDriver with the AI Thinker pins and RGB565 HVGA frames.
     */
    CameraDriver();

    /**
     * @brief Initializes the camera with the configured parameters.
     * @return true if camera initialization succeeded, false otherwise.
     */
    bool initialize();

    /**
     * @brief Captures a frame and converts it to JPEG.
     * @param jpegBuffer Receives the JPEG buffer; release it with releaseJpeg().
     * @param jpegLength Receives the JPEG size in bytes.
     * @return true if the image was captured and converted, false otherwise.
     */
    bool captureJpeg(uint8_t** jpegBuffer, size_t* jpegLength);

    /**
     * @brief Releases a JPEG buffer returned by captureJpeg().
     * @param jpegBuffer The buffer to release.
     */
    void releaseJpeg(uint8_t* jpegBuffer);

    /**
     * @brief Checks if the camera is initialized and ready.
     * @return true if the camera is initialized, false otherwise.
     */
    [[nodiscard]] bool isInitialized() const;

private:
    camera_config_t cameraConfig_;
    bool initialized_ = false;
};

#endif /* CameraDriver_h */