/**
 * @file CameraDriver.cpp
 * @brief Implementation of the CameraDriver wrapper for the ESP32-CAM image sensor.
 */

#include "CameraDriver.h"
#include <Arduino.h>
#include <img_converters.h>

/** @name AI Thinker ESP32-CAM Pin Definitions */
//@{
static const int CAM_PIN_PWDN = 32;
static const int CAM_PIN_RESET = -1;
static const int CAM_PIN_XCLK = 0;
static const int CAM_PIN_SIOD = 26;
static const int CAM_PIN_SIOC = 27;
static const int CAM_PIN_D7 = 35;
static const int CAM_PIN_D6 = 34;
static const int CAM_PIN_D5 = 39;
static const int CAM_PIN_D4 = 36;
static const int CAM_PIN_D3 = 21;
static const int CAM_PIN_D2 = 19;
static const int CAM_PIN_D1 = 18;
static const int CAM_PIN_D0 = 5;
static const int CAM_PIN_VSYNC = 25;
static const int CAM_PIN_HREF = 23;
static const int CAM_PIN_PCLK = 22;
//@}

/** @name Camera Configuration */
//@{
static const uint32_t CAM_XCLK_FREQ_HZ = 20000000;
static const uint8_t CAM_FRAME_BUFFER_COUNT = 1;
static const uint8_t JPEG_CONVERSION_QUALITY = 80;
//@}

CameraDriver::CameraDriver() {
    cameraConfig_.pin_pwdn = CAM_PIN_PWDN;
    cameraConfig_.pin_reset = CAM_PIN_RESET;
    cameraConfig_.pin_xclk = CAM_PIN_XCLK;
    cameraConfig_.pin_sccb_sda = CAM_PIN_SIOD;
    cameraConfig_.pin_sccb_scl = CAM_PIN_SIOC;
    cameraConfig_.pin_d7 = CAM_PIN_D7;
    cameraConfig_.pin_d6 = CAM_PIN_D6;
    cameraConfig_.pin_d5 = CAM_PIN_D5;
    cameraConfig_.pin_d4 = CAM_PIN_D4;
    cameraConfig_.pin_d3 = CAM_PIN_D3;
    cameraConfig_.pin_d2 = CAM_PIN_D2;
    cameraConfig_.pin_d1 = CAM_PIN_D1;
    cameraConfig_.pin_d0 = CAM_PIN_D0;
    cameraConfig_.pin_vsync = CAM_PIN_VSYNC;
    cameraConfig_.pin_href = CAM_PIN_HREF;
    cameraConfig_.pin_pclk = CAM_PIN_PCLK;
    cameraConfig_.xclk_freq_hz = CAM_XCLK_FREQ_HZ;
    cameraConfig_.ledc_timer = LEDC_TIMER_0;
    cameraConfig_.ledc_channel = LEDC_CHANNEL_0;
    cameraConfig_.pixel_format = PIXFORMAT_RGB565;
    cameraConfig_.frame_size = FRAMESIZE_HVGA;
    cameraConfig_.jpeg_quality = 12;
    cameraConfig_.fb_count = CAM_FRAME_BUFFER_COUNT;
    cameraConfig_.fb_location = CAMERA_FB_IN_PSRAM;
    cameraConfig_.grab_mode = CAMERA_GRAB_LATEST;
}

bool CameraDriver::initialize() {
    esp_err_t err = esp_camera_init(&cameraConfig_);
    if (err != ESP_OK) {
        Serial.printf("[CameraDriver] Camera init failed: 0x%x\n", err);
        return false;
    }
    initialized_ = true;
    Serial.println("[CameraDriver] Camera initialized");
    return true;
}

bool CameraDriver::captureJpeg(uint8_t** jpegBuffer, size_t* jpegLength) {
    if (!initialized_) {
        Serial.println("[CameraDriver] Camera not initialized");
        return false;
    }

    camera_fb_t* frame = esp_camera_fb_get();
    if (frame == nullptr) {
        Serial.println("[CameraDriver] Failed to capture frame");
        return false;
    }

    bool converted = frame2jpg(frame, JPEG_CONVERSION_QUALITY, jpegBuffer, jpegLength);
    Serial.printf("[CameraDriver] Frame %ux%u captured\n", frame->width, frame->height);
    esp_camera_fb_return(frame);

    if (!converted) {
        Serial.println("[CameraDriver] JPEG conversion failed");
        return false;
    }
    return true;
}

void CameraDriver::releaseJpeg(uint8_t* jpegBuffer) {
    if (jpegBuffer) {
        free(jpegBuffer);
    }
}

bool CameraDriver::isInitialized() const {
    return initialized_;
}