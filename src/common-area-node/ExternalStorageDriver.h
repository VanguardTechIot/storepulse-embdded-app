/**
 * @file ExternalStorageDriver.h
 * @brief Wrapper around SD_MMC for external SD card file operations.
 * @copyright (c) 2025-2026 Angel Velasquez
 * @license Apache-2.0
 *
 * @par Modest IoT Nano Framework aspects illustrated:
 * - Driver pattern: Low-level protocol abstraction, similar to Hd44780Driver/Ssd1306Driver
 * - NOT part of the framework: SD_MMC is board-specific (ESP32-CAM has SD slot; most boards do not)
 * - Separation of concerns: File I/O is independent of camera capture logic
 *
 * @note This class is sketch-level only. It wraps the SD_MMC ESP32 built-in library
 *       and should be defined in the application sketch, not in the framework.
 */

#ifndef ExternalStorageDriver_h
#define ExternalStorageDriver_h

#include <Arduino.h>

/**
 * @brief Wrapper around SD_MMC for external SD card file operations.
 *
 * This class follows the Driver pattern used by Hd44780Driver and Ssd1306Driver:
 * a low-level protocol abstraction that handles hardware-specific communication.
 * Unlike those drivers, ExternalStorageDriver is NOT part of the Modest IoT Nano
 * Framework because SD_MMC is board-specific (ESP32-CAM has an SD card slot;
 * most ESP32 boards do not).
 *
 * The class provides file I/O operations for large binary files (camera images,
 * audio, video) that don't fit in the ESP32's internal flash. For small, frequent
 * writes (telemetry, configuration), the framework uses ConfigurationStore (NVS)
 * and the planned TelemetryBuffer (LittleFS).
 *
 * @par Storage hierarchy:
 * - ConfigurationStore (NVS): Key-value persistence for WiFi/MQTT credentials
 * - TelemetryBuffer (LittleFS, W25): JSON telemetry persistence for offline replay
 * - ExternalStorageDriver (SD_MMC): Large binary files (images, audio, video)
 */
class ExternalStorageDriver {
public:
    /**
     * @brief Constructs an ExternalStorageDriver.
     */
    ExternalStorageDriver();

    /**
     * @brief Initializes the SD card via MMC interface.
     * @param mountPath The mount point (e.g., "/sdcard").
     * @param useOneLineSdMode true for one-line SD mode (default for ESP32-CAM).
     * @param formatIfMounted false to prevent automatic formatting.
     * @return true if SD card initialization succeeded, false otherwise.
     * @note Must be called before any file operations.
     */
    bool initialize(const char* mountPath, bool useOneLineSdMode, bool formatIfMounted);

    /**
     * @brief Writes binary data to a file on the SD card.
     * @param path The file path (e.g., "/motion_capture.jpg").
     * @param data Pointer to the binary data buffer.
     * @param length Number of bytes to write.
     * @return true if the write succeeded, false otherwise.
     * @note The file is created or overwritten. Use append() for appending.
     */
    bool writeBinaryFile(const char* path, const uint8_t* data, size_t length);

    /**
     * @brief Reads binary data from a file on the SD card.
     * @param path The file path (e.g., "/motion_capture.jpg").
     * @param buffer Pointer to the output buffer.
     * @param maxLength Maximum number of bytes to read.
     * @return Number of bytes read, or 0 if the file was not found.
     * @note The caller is responsible for allocating a sufficiently large buffer.
     */
    [[nodiscard]] size_t readBinaryFile(const char* path, uint8_t* buffer, size_t maxLength);

    /**
     * @brief Checks if a file exists on the SD card.
     * @param path The file path (e.g., "/motion_capture.jpg").
     * @return true if the file exists, false otherwise.
     */
    [[nodiscard]] bool fileExists(const char* path);

    /**
     * @brief Deletes a file from the SD card.
     * @param path The file path (e.g., "/motion_capture.jpg").
     * @return true if the file was deleted, false otherwise.
     */
    bool deleteFile(const char* path);

    /**
     * @brief Returns the total capacity of the SD card.
     * @return Total capacity in bytes.
     */
    [[nodiscard]] uint64_t totalBytes() const;

    /**
     * @brief Returns the used space on the SD card.
     * @return Used space in bytes.
     */
    [[nodiscard]] uint64_t usedBytes() const;

    /**
     * @brief Checks if the SD card is initialized and ready.
     * @return true if the SD card is initialized, false otherwise.
     */
    [[nodiscard]] bool isInitialized() const;

private:
    bool initialized_ = false;
};

#endif /* ExternalStorageDriver_h */