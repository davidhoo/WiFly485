#ifndef DEBUG_UTILS_H
#define DEBUG_UTILS_H

#include <Arduino.h>

// 调试级别定义
enum DebugLevel {
    DEBUG_NONE = 0,
    DEBUG_ERROR = 1,
    DEBUG_WARNING = 2,
    DEBUG_INFO = 3,
    DEBUG_VERBOSE = 4
};

// 调试宏定义
#ifndef DEBUG_LEVEL
#define DEBUG_LEVEL DEBUG_INFO
#endif

#define DEBUG_PRINT(level, format, ...) \
    do { \
        if (level <= DEBUG_LEVEL) { \
            Serial.printf("[%s] " format "\n", getDebugLevelString(level), ##__VA_ARGS__); \
        } \
    } while(0)

#define DEBUG_ERROR_PRINT(format, ...) DEBUG_PRINT(DEBUG_ERROR, format, ##__VA_ARGS__)
#define DEBUG_WARNING_PRINT(format, ...) DEBUG_PRINT(DEBUG_WARNING, format, ##__VA_ARGS__)
#define DEBUG_INFO_PRINT(format, ...) DEBUG_PRINT(DEBUG_INFO, format, ##__VA_ARGS__)
#define DEBUG_VERBOSE_PRINT(format, ...) DEBUG_PRINT(DEBUG_VERBOSE, format, ##__VA_ARGS__)

const char* getDebugLevelString(DebugLevel level);
void printSystemInfo();
void printMemoryInfo();

#endif