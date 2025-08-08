#include "debug_utils.h"
#include <ESP8266WiFi.h>

const char* getDebugLevelString(DebugLevel level) {
    switch (level) {
        case DEBUG_ERROR: return "ERROR";
        case DEBUG_WARNING: return "WARN";
        case DEBUG_INFO: return "INFO";
        case DEBUG_VERBOSE: return "VERB";
        default: return "NONE";
    }
}

void printSystemInfo() {
    DEBUG_INFO_PRINT("=== System Information ===");
#ifdef DEVICE_ROLE_MASTER
    DEBUG_INFO_PRINT("Device Name: WiFly485_Master");
#else
    DEBUG_INFO_PRINT("Device Name: WiFly485_Slave");
#endif
    
#ifdef DEVICE_ROLE_MASTER
    DEBUG_INFO_PRINT("Device Role: Master");
#else
    DEBUG_INFO_PRINT("Device Role: Slave");
#endif

    DEBUG_INFO_PRINT("Chip ID: %08X", ESP.getChipId());
    DEBUG_INFO_PRINT("Flash Chip ID: %08X", ESP.getFlashChipId());
    DEBUG_INFO_PRINT("Flash Size: %d bytes", ESP.getFlashChipSize());
    DEBUG_INFO_PRINT("Free Sketch Space: %d bytes", ESP.getFreeSketchSpace());
    DEBUG_INFO_PRINT("CPU Frequency: %d MHz", ESP.getCpuFreqMHz());
    DEBUG_INFO_PRINT("SDK Version: %s", ESP.getSdkVersion());
    DEBUG_INFO_PRINT("Boot Version: %d", ESP.getBootVersion());
    DEBUG_INFO_PRINT("Boot Mode: %d", ESP.getBootMode());
    DEBUG_INFO_PRINT("Reset Reason: %s", ESP.getResetReason().c_str());
    DEBUG_INFO_PRINT("Reset Info: %s", ESP.getResetInfo().c_str());
    printMemoryInfo();
    DEBUG_INFO_PRINT("========================");
}

void printMemoryInfo() {
    uint32_t freeHeap = ESP.getFreeHeap();
    uint32_t heapFragmentation = ESP.getHeapFragmentation();
    uint32_t maxFreeBlockSize = ESP.getMaxFreeBlockSize();
    
    DEBUG_INFO_PRINT("Free Heap: %d bytes", freeHeap);
    DEBUG_INFO_PRINT("Heap Fragmentation: %d%%", heapFragmentation);
    DEBUG_INFO_PRINT("Max Free Block: %d bytes", maxFreeBlockSize);
    
    // 计算内存使用率（假设总内存约80KB）
    float memoryUsage = (80000 - freeHeap) / 800.0f;
    DEBUG_INFO_PRINT("Memory Usage: %.1f%%", memoryUsage);
}