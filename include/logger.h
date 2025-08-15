#ifndef LOGGER_H
#define LOGGER_H

#include <Arduino.h>

// 日志级别定义
enum LogLevel {
  LOG_LEVEL_NONE = 0,
  LOG_LEVEL_ERROR = 1,
  LOG_LEVEL_WARN = 2,
  LOG_LEVEL_INFO = 3,
  LOG_LEVEL_DEBUG = 4,
  LOG_LEVEL_VERBOSE = 5
};

class Logger {
public:
  Logger();
  ~Logger();

  // 初始化日志系统
  void begin();
  
  // 设置日志级别
  void setLogLevel(LogLevel level);
  
  // 获取当前日志级别
  LogLevel getLogLevel();
  
  // 日志输出函数
  void log(LogLevel level, const char* tag, const char* format, ...);
  
  // 不同级别的日志输出函数
  void error(const char* tag, const char* format, ...);
  void warn(const char* tag, const char* format, ...);
  void info(const char* tag, const char* format, ...);
  void debug(const char* tag, const char* format, ...);
  void verbose(const char* tag, const char* format, ...);

private:
  LogLevel currentLogLevel;
  
  // 获取日志级别字符串
  const char* getLogLevelString(LogLevel level);
  
  // 内部日志输出函数
  void logInternal(LogLevel level, const char* tag, const char* format, va_list args);
};

// 全局日志实例
extern Logger logger;

// 日志宏定义，方便使用
#define LOG_E(tag, format, ...) logger.error(tag, format, ##__VA_ARGS__)
#define LOG_W(tag, format, ...) logger.warn(tag, format, ##__VA_ARGS__)
#define LOG_I(tag, format, ...) logger.info(tag, format, ##__VA_ARGS__)
#define LOG_D(tag, format, ...) logger.debug(tag, format, ##__VA_ARGS__)
#define LOG_V(tag, format, ...) logger.verbose(tag, format, ##__VA_ARGS__)

#endif // LOGGER_H