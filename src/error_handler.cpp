#include "error_handler.h"
#include <ESP8266WiFi.h>

// 全局错误处理器实例
ErrorHandler errorHandler;

ErrorHandler::ErrorHandler() : errorCount(0), currentIndex(0) {
  // 初始化错误历史数组
  for (int i = 0; i < MAX_ERROR_HISTORY; i++) {
    errorHistory[i].code = ERROR_NONE;
    errorHistory[i].severity = ERROR_SEVERITY_INFO;
    errorHistory[i].module = "";
    errorHistory[i].message = "";
    errorHistory[i].timestamp = 0;
  }
}

ErrorHandler::~ErrorHandler() {
  // 析构函数
}

void ErrorHandler::begin() {
  // 初始化错误处理器
  LOG_I("ErrorHandler", "Error handler initialized");
}

void ErrorHandler::reportError(ErrorCode code, const String& module, const String& message) {
  // 使用默认严重级别报告错误
  ErrorSeverity severity = getDefaultSeverity(code);
  reportError(code, module, message, severity);
}

void ErrorHandler::reportError(ErrorCode code, const String& module, const String& message, ErrorSeverity severity) {
  // 创建错误信息
  ErrorInfo error;
  error.code = code;
  error.severity = severity;
  error.module = module;
  error.message = message;
  error.timestamp = millis();
  
  // 添加到错误历史
  errorHistory[currentIndex] = error;
  currentIndex = (currentIndex + 1) % MAX_ERROR_HISTORY;
  
  // 更新错误计数
  if (errorCount < MAX_ERROR_HISTORY) {
    errorCount++;
  }
  
  // 记录错误日志
  logError(error);
}

ErrorInfo ErrorHandler::getLastError() {
  // 获取最近的错误
  if (errorCount == 0) {
    ErrorInfo emptyError;
    emptyError.code = ERROR_NONE;
    emptyError.severity = ERROR_SEVERITY_INFO;
    emptyError.module = "";
    emptyError.message = "";
    emptyError.timestamp = 0;
    return emptyError;
  }
  
  int lastIndex = (currentIndex - 1 + MAX_ERROR_HISTORY) % MAX_ERROR_HISTORY;
  return errorHistory[lastIndex];
}

ErrorInfo ErrorHandler::getErrorByIndex(int index) {
  // 根据索引获取错误
  if (index < 0 || index >= errorCount) {
    ErrorInfo emptyError;
    emptyError.code = ERROR_NONE;
    emptyError.severity = ERROR_SEVERITY_INFO;
    emptyError.module = "";
    emptyError.message = "";
    emptyError.timestamp = 0;
    return emptyError;
  }
  
  int actualIndex = (currentIndex - errorCount + index + MAX_ERROR_HISTORY) % MAX_ERROR_HISTORY;
  return errorHistory[actualIndex];
}

int ErrorHandler::getErrorCount() {
  // 获取错误数量
  return errorCount;
}

void ErrorHandler::clearErrors() {
  // 清除所有错误
  for (int i = 0; i < MAX_ERROR_HISTORY; i++) {
    errorHistory[i].code = ERROR_NONE;
    errorHistory[i].severity = ERROR_SEVERITY_INFO;
    errorHistory[i].module = "";
    errorHistory[i].message = "";
    errorHistory[i].timestamp = 0;
  }
  errorCount = 0;
  currentIndex = 0;
}

String ErrorHandler::getErrorCodeString(ErrorCode code) {
  // 获取错误代码描述
  switch (code) {
    case ERROR_NONE:
      return "No Error";
    case ERROR_UNKNOWN:
      return "Unknown Error";
    case ERROR_INVALID_PARAMETER:
      return "Invalid Parameter";
    case ERROR_OUT_OF_MEMORY:
      return "Out of Memory";
    case ERROR_TIMEOUT:
      return "Timeout";
    case ERROR_NOT_IMPLEMENTED:
      return "Not Implemented";
    case ERROR_WIFI_NOT_CONNECTED:
      return "WiFi Not Connected";
    case ERROR_WIFI_CONNECTION_FAILED:
      return "WiFi Connection Failed";
    case ERROR_WIFI_DHCP_FAILED:
      return "WiFi DHCP Failed";
    case ERROR_TCP_CONNECTION_FAILED:
      return "TCP Connection Failed";
    case ERROR_TCP_SEND_FAILED:
      return "TCP Send Failed";
    case ERROR_TCP_RECEIVE_FAILED:
      return "TCP Receive Failed";
    case ERROR_MDNS_FAILED:
      return "mDNS Failed";
    case ERROR_RS485_BUFFER_OVERFLOW:
      return "RS485 Buffer Overflow";
    case ERROR_RS485_COMMUNICATION_FAILED:
      return "RS485 Communication Failed";
    case ERROR_RS485_INVALID_DATA:
      return "RS485 Invalid Data";
    case ERROR_CONFIG_LOAD_FAILED:
      return "Config Load Failed";
    case ERROR_CONFIG_SAVE_FAILED:
      return "Config Save Failed";
    case ERROR_CONFIG_INVALID:
      return "Config Invalid";
    case ERROR_CONFIG_SYNC_FAILED:
      return "Config Sync Failed";
    case ERROR_FILE_NOT_FOUND:
      return "File Not Found";
    case ERROR_FILE_READ_FAILED:
      return "File Read Failed";
    case ERROR_FILE_WRITE_FAILED:
      return "File Write Failed";
    case ERROR_FILESYSTEM_MOUNT_FAILED:
      return "Filesystem Mount Failed";
    case ERROR_SYSTEM_INIT_FAILED:
      return "System Initialization Failed";
    case ERROR_SYSTEM_BUSY:
      return "System Busy";
    default:
      return "Unknown Error Code";
  }
}

String ErrorHandler::getErrorSeverityString(ErrorSeverity severity) {
  // 获取错误严重级别描述
  switch (severity) {
    case ERROR_SEVERITY_INFO:
      return "Info";
    case ERROR_SEVERITY_WARNING:
      return "Warning";
    case ERROR_SEVERITY_ERROR:
      return "Error";
    case ERROR_SEVERITY_CRITICAL:
      return "Critical";
    default:
      return "Unknown";
  }
}

void ErrorHandler::setGlobalErrorHandler(ErrorHandler* handler) {
  // 设置全局错误处理器
  // 注意：在当前实现中，我们使用全局实例，这个函数主要用于未来扩展
}

ErrorHandler* ErrorHandler::getGlobalErrorHandler() {
  // 获取全局错误处理器
  return &errorHandler;
}

void ErrorHandler::logError(const ErrorInfo& error) {
  // 记录错误日志
  String severityStr = getErrorSeverityString(error.severity);
  String codeStr = getErrorCodeString(error.code);
  
  switch (error.severity) {
    case ERROR_SEVERITY_INFO:
      LOG_I("ErrorHandler", "[%s] Module: %s, Code: %s, Message: %s", 
            severityStr.c_str(), error.module.c_str(), codeStr.c_str(), error.message.c_str());
      break;
    case ERROR_SEVERITY_WARNING:
      LOG_W("ErrorHandler", "[%s] Module: %s, Code: %s, Message: %s", 
            severityStr.c_str(), error.module.c_str(), codeStr.c_str(), error.message.c_str());
      break;
    case ERROR_SEVERITY_ERROR:
      LOG_E("ErrorHandler", "[%s] Module: %s, Code: %s, Message: %s", 
            severityStr.c_str(), error.module.c_str(), codeStr.c_str(), error.message.c_str());
      break;
    case ERROR_SEVERITY_CRITICAL:
      LOG_E("ErrorHandler", "[CRITICAL] Module: %s, Code: %s, Message: %s", 
            error.module.c_str(), codeStr.c_str(), error.message.c_str());
      break;
    default:
      LOG_I("ErrorHandler", "[%s] Module: %s, Code: %s, Message: %s", 
            severityStr.c_str(), error.module.c_str(), codeStr.c_str(), error.message.c_str());
      break;
  }
}

ErrorSeverity ErrorHandler::getDefaultSeverity(ErrorCode code) {
  // 根据错误代码获取默认严重级别
  switch (code) {
    case ERROR_NONE:
      return ERROR_SEVERITY_INFO;
    case ERROR_UNKNOWN:
    case ERROR_INVALID_PARAMETER:
    case ERROR_TIMEOUT:
      return ERROR_SEVERITY_WARNING;
    case ERROR_OUT_OF_MEMORY:
    case ERROR_WIFI_NOT_CONNECTED:
    case ERROR_WIFI_DHCP_FAILED:
    case ERROR_TCP_CONNECTION_FAILED:
    case ERROR_RS485_BUFFER_OVERFLOW:
    case ERROR_CONFIG_INVALID:
    case ERROR_FILE_NOT_FOUND:
    case ERROR_FILE_READ_FAILED:
    case ERROR_FILE_WRITE_FAILED:
      return ERROR_SEVERITY_ERROR;
    case ERROR_WIFI_CONNECTION_FAILED:
    case ERROR_TCP_SEND_FAILED:
    case ERROR_TCP_RECEIVE_FAILED:
    case ERROR_MDNS_FAILED:
    case ERROR_RS485_COMMUNICATION_FAILED:
    case ERROR_RS485_INVALID_DATA:
    case ERROR_CONFIG_LOAD_FAILED:
    case ERROR_CONFIG_SAVE_FAILED:
    case ERROR_CONFIG_SYNC_FAILED:
    case ERROR_FILESYSTEM_MOUNT_FAILED:
    case ERROR_SYSTEM_INIT_FAILED:
    case ERROR_SYSTEM_BUSY:
    case ERROR_NOT_IMPLEMENTED:
      return ERROR_SEVERITY_CRITICAL;
    default:
      return ERROR_SEVERITY_ERROR;
  }
}