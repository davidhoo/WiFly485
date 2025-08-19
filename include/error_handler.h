#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H

#include <Arduino.h>
#include "logger.h"

// 错误代码枚举
enum ErrorCode {
  // 通用错误代码
  ERROR_NONE = 0,
  ERROR_UNKNOWN = 1,
  ERROR_INVALID_PARAMETER = 2,
  ERROR_OUT_OF_MEMORY = 3,
  ERROR_TIMEOUT = 4,
  ERROR_NOT_IMPLEMENTED = 5,
  
  // 网络相关错误代码
  ERROR_WIFI_NOT_CONNECTED = 100,
  ERROR_WIFI_CONNECTION_FAILED = 101,
  ERROR_WIFI_DHCP_FAILED = 102,
  ERROR_TCP_CONNECTION_FAILED = 103,
  ERROR_TCP_SEND_FAILED = 104,
  ERROR_TCP_RECEIVE_FAILED = 105,
  ERROR_MDNS_FAILED = 106,
  
  // RS485相关错误代码
  ERROR_RS485_BUFFER_OVERFLOW = 200,
  ERROR_RS485_COMMUNICATION_FAILED = 201,
  ERROR_RS485_INVALID_DATA = 202,
  
  // 配置相关错误代码
  ERROR_CONFIG_LOAD_FAILED = 300,
  ERROR_CONFIG_SAVE_FAILED = 301,
  ERROR_CONFIG_INVALID = 302,
  ERROR_CONFIG_SYNC_FAILED = 303,
  
  // 文件系统相关错误代码
  ERROR_FILE_NOT_FOUND = 400,
  ERROR_FILE_READ_FAILED = 401,
  ERROR_FILE_WRITE_FAILED = 402,
  ERROR_FILESYSTEM_MOUNT_FAILED = 403,
  
  // 系统相关错误代码
  ERROR_SYSTEM_INIT_FAILED = 500,
  ERROR_SYSTEM_BUSY = 501
};

// 错误严重级别枚举
enum ErrorSeverity {
  ERROR_SEVERITY_INFO = 0,
  ERROR_SEVERITY_WARNING = 1,
  ERROR_SEVERITY_ERROR = 2,
  ERROR_SEVERITY_CRITICAL = 3
};

// 错误信息结构体
struct ErrorInfo {
  ErrorCode code;
  ErrorSeverity severity;
  String module;
  String message;
  unsigned long timestamp;
};

class ErrorHandler {
public:
  ErrorHandler();
  ~ErrorHandler();
  
  // 初始化错误处理器
  void begin();
  
  // 报告错误
  void reportError(ErrorCode code, const String& module, const String& message);
  void reportError(ErrorCode code, const String& module, const String& message, ErrorSeverity severity);
  
  // 获取最近的错误
  ErrorInfo getLastError();
  ErrorInfo getErrorByIndex(int index);
  
  // 获取错误数量
  int getErrorCount();
  
  // 清除错误
  void clearErrors();
  
  // 获取错误代码描述
  String getErrorCodeString(ErrorCode code);
  
  // 获取错误严重级别描述
  String getErrorSeverityString(ErrorSeverity severity);
  
  // 设置全局错误处理器
  static void setGlobalErrorHandler(ErrorHandler* handler);
  static ErrorHandler* getGlobalErrorHandler();
  
  // 获取默认严重级别
  ErrorSeverity getDefaultSeverity(ErrorCode code);
  
private:
  static const int MAX_ERROR_HISTORY = 10;
  ErrorInfo errorHistory[MAX_ERROR_HISTORY];
  int errorCount;
  int currentIndex;
  
  // 内部辅助函数
  void logError(const ErrorInfo& error);
};

// 全局错误处理器实例
extern ErrorHandler errorHandler;

// 错误处理宏定义
#define REPORT_ERROR(code, module, message) \
  ErrorHandler::getGlobalErrorHandler()->reportError(code, module, message)
  
#define REPORT_ERROR_SEVERITY(code, module, message, severity) \
  ErrorHandler::getGlobalErrorHandler()->reportError(code, module, message, severity)

#endif // ERROR_HANDLER_H