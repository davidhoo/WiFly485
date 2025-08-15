#include "logger.h"
#include <ESP8266WiFi.h>

// 全局日志实例
Logger logger;

Logger::Logger() : currentLogLevel(LOG_LEVEL_INFO) {
  // 构造函数
}

Logger::~Logger() {
  // 析构函数
}

void Logger::begin() {
  // 初始化日志系统
  Serial.begin(115200);
  delay(100);
  
  Serial.println("\n=== WiFly485 日志系统初始化 ===");
}

void Logger::setLogLevel(LogLevel level) {
  currentLogLevel = level;
}

LogLevel Logger::getLogLevel() {
  return currentLogLevel;
}

void Logger::log(LogLevel level, const char* tag, const char* format, ...) {
  if (level > currentLogLevel) {
    return;
  }
  
  va_list args;
  va_start(args, format);
  logInternal(level, tag, format, args);
  va_end(args);
}

void Logger::error(const char* tag, const char* format, ...) {
  if (LOG_LEVEL_ERROR > currentLogLevel) {
    return;
  }
  
  va_list args;
  va_start(args, format);
  logInternal(LOG_LEVEL_ERROR, tag, format, args);
  va_end(args);
}

void Logger::warn(const char* tag, const char* format, ...) {
  if (LOG_LEVEL_WARN > currentLogLevel) {
    return;
  }
  
  va_list args;
  va_start(args, format);
  logInternal(LOG_LEVEL_WARN, tag, format, args);
  va_end(args);
}

void Logger::info(const char* tag, const char* format, ...) {
  if (LOG_LEVEL_INFO > currentLogLevel) {
    return;
  }
  
  va_list args;
  va_start(args, format);
  logInternal(LOG_LEVEL_INFO, tag, format, args);
  va_end(args);
}

void Logger::debug(const char* tag, const char* format, ...) {
  if (LOG_LEVEL_DEBUG > currentLogLevel) {
    return;
  }
  
  va_list args;
  va_start(args, format);
  logInternal(LOG_LEVEL_DEBUG, tag, format, args);
  va_end(args);
}

void Logger::verbose(const char* tag, const char* format, ...) {
  if (LOG_LEVEL_VERBOSE > currentLogLevel) {
    return;
  }
  
  va_list args;
  va_start(args, format);
  logInternal(LOG_LEVEL_VERBOSE, tag, format, args);
  va_end(args);
}

const char* Logger::getLogLevelString(LogLevel level) {
  switch (level) {
    case LOG_LEVEL_ERROR:
      return "E";
    case LOG_LEVEL_WARN:
      return "W";
    case LOG_LEVEL_INFO:
      return "I";
    case LOG_LEVEL_DEBUG:
      return "D";
    case LOG_LEVEL_VERBOSE:
      return "V";
    default:
      return "?";
  }
}

void Logger::logInternal(LogLevel level, const char* tag, const char* format, va_list args) {
  // 获取时间戳
  unsigned long timestamp = millis();
  
  // 输出日志级别、时间戳和标签
  Serial.printf("[%s][%lu][%s] ", getLogLevelString(level), timestamp, tag);
  
  // 输出格式化消息
  char buffer[256];
  vsnprintf(buffer, sizeof(buffer), format, args);
  Serial.println(buffer);
}