/**
 * @file ExternalStorageDriver.cpp
 * @brief Implementation of ExternalStorageDriver wrapper for SD_MMC file operations.
 * @copyright (c) 2025-2026 Angel Velasquez
 * @license Apache-2.0
 */

#include "ExternalStorageDriver.h"
#include <SD_MMC.h>

ExternalStorageDriver::ExternalStorageDriver() {}

bool ExternalStorageDriver::initialize(const char* mountPath, bool useOneLineSdMode,
                                       bool formatIfMounted) {
    if (!SD_MMC.begin(mountPath, useOneLineSdMode, formatIfMounted)) {
        Serial.println("[ExternalStorageDriver] SD card initialization failed!");
        return false;
    }
    initialized_ = true;
    Serial.println("[ExternalStorageDriver] SD card initialized");
    return true;
}

bool ExternalStorageDriver::writeBinaryFile(const char* path, const uint8_t* data,
                                            size_t length) {
    if (!initialized_ || !path || !data || length == 0) {
        return false;
    }

    File file = SD_MMC.open(path, FILE_WRITE);
    if (!file) {
        Serial.printf("[ExternalStorageDriver] Failed to open file for writing: %s\n", path);
        return false;
    }

    size_t bytesWritten = file.write(data, length);
    file.close();

    if (bytesWritten != length) {
        Serial.printf("[ExternalStorageDriver] Write error: %d/%d bytes for %s\n",
                      bytesWritten, length, path);
        return false;
    }

    Serial.printf("[ExternalStorageDriver] Wrote %d bytes to %s\n", length, path);
    return true;
}

size_t ExternalStorageDriver::readBinaryFile(const char* path, uint8_t* buffer,
                                             size_t maxLength) {
    if (!initialized_ || !path || !buffer || maxLength == 0) {
        return 0;
    }

    File file = SD_MMC.open(path, FILE_READ);
    if (!file) {
        Serial.printf("[ExternalStorageDriver] Failed to open file for reading: %s\n", path);
        return 0;
    }

    size_t bytesRead = file.read(buffer, maxLength);
    file.close();

    Serial.printf("[ExternalStorageDriver] Read %d bytes from %s\n", bytesRead, path);
    return bytesRead;
}

bool ExternalStorageDriver::fileExists(const char* path) {
    if (!initialized_ || !path) {
        return false;
    }
    return SD_MMC.exists(path);
}

bool ExternalStorageDriver::deleteFile(const char* path) {
    if (!initialized_ || !path) {
        return false;
    }
    bool result = SD_MMC.remove(path);
    if (result) {
        Serial.printf("[ExternalStorageDriver] Deleted file: %s\n", path);
    } else {
        Serial.printf("[ExternalStorageDriver] Failed to delete file: %s\n", path);
    }
    return result;
}

uint64_t ExternalStorageDriver::totalBytes() const {
    if (!initialized_) {
        return 0;
    }
    return SD_MMC.totalBytes();
}

uint64_t ExternalStorageDriver::usedBytes() const {
    if (!initialized_) {
        return 0;
    }
    return SD_MMC.usedBytes();
}

bool ExternalStorageDriver::isInitialized() const {
    return initialized_;
}