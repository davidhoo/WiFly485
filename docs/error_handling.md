# WiFly485 错误处理框架文档

## 1. 概述

WiFly485 错误处理框架是一个统一的错误管理机制，用于在整个项目中一致地处理、记录和报告错误。该框架提供了标准化的错误代码、严重级别分类和日志记录功能。

## 2. 错误代码定义

### 2.1 通用错误代码
- `ERROR_NONE` (0): 无错误
- `ERROR_UNKNOWN` (1): 未知错误
- `ERROR_INVALID_PARAMETER` (2): 无效参数
- `ERROR_OUT_OF_MEMORY` (3): 内存不足
- `ERROR_TIMEOUT` (4): 超时
- `ERROR_NOT_IMPLEMENTED` (5): 未实现

### 2.2 网络相关错误代码
- `ERROR_WIFI_NOT_CONNECTED` (100): WiFi未连接
- `ERROR_WIFI_CONNECTION_FAILED` (101): WiFi连接失败
- `ERROR_WIFI_DHCP_FAILED` (102): WiFi DHCP失败
- `ERROR_TCP_CONNECTION_FAILED` (103): TCP连接失败
- `ERROR_TCP_SEND_FAILED` (104): TCP发送失败
- `ERROR_TCP_RECEIVE_FAILED` (105): TCP接收失败
- `ERROR_MDNS_FAILED` (106): mDNS失败

### 2.3 RS485相关错误代码
- `ERROR_RS485_BUFFER_OVERFLOW` (200): RS485缓冲区溢出
- `ERROR_RS485_COMMUNICATION_FAILED` (201): RS485通信失败
- `ERROR_RS485_INVALID_DATA` (202): RS485无效数据

### 2.4 配置相关错误代码
- `ERROR_CONFIG_LOAD_FAILED` (300): 配置加载失败
- `ERROR_CONFIG_SAVE_FAILED` (301): 配置保存失败
- `ERROR_CONFIG_INVALID` (302): 配置无效
- `ERROR_CONFIG_SYNC_FAILED` (303): 配置同步失败

### 2.5 文件系统相关错误代码
- `ERROR_FILE_NOT_FOUND` (400): 文件未找到
- `ERROR_FILE_READ_FAILED` (401): 文件读取失败
- `ERROR_FILE_WRITE_FAILED` (402): 文件写入失败
- `ERROR_FILESYSTEM_MOUNT_FAILED` (403): 文件系统挂载失败

### 2.6 系统相关错误代码
- `ERROR_SYSTEM_INIT_FAILED` (500): 系统初始化失败
- `ERROR_SYSTEM_BUSY` (501): 系统忙

## 3. 错误严重级别

- `ERROR_SEVERITY_INFO` (0): 信息级别
- `ERROR_SEVERITY_WARNING` (1): 警告级别
- `ERROR_SEVERITY_ERROR` (2): 错误级别
- `ERROR_SEVERITY_CRITICAL` (3): 严重错误级别

## 4. 错误处理器类

### 4.1 类定义
```cpp
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
};
```

### 4.2 错误信息结构体
```cpp
struct ErrorInfo {
  ErrorCode code;
  ErrorSeverity severity;
  String module;
  String message;
  unsigned long timestamp;
};
```

## 5. 使用方法

### 5.1 初始化错误处理器
在主程序中初始化错误处理器：
```cpp
#include "error_handler.h"

ErrorHandler errorHandler;

void setup() {
  errorHandler.begin();
  ErrorHandler::setGlobalErrorHandler(&errorHandler);
}
```

### 5.2 报告错误
使用宏定义报告错误：
```cpp
// 报告错误，使用默认严重级别
REPORT_ERROR(ERROR_INVALID_PARAMETER, "ModuleName", "Invalid parameter provided");

// 报告错误，指定严重级别
REPORT_ERROR_SEVERITY(ERROR_WIFI_CONNECTION_FAILED, "WiFiManager", "Failed to connect to WiFi", ERROR_SEVERITY_CRITICAL);
```

### 5.3 获取错误信息
```cpp
// 获取最近的错误
ErrorInfo lastError = errorHandler.getLastError();

// 获取错误数量
int errorCount = errorHandler.getErrorCount();

// 获取指定索引的错误
ErrorInfo error = errorHandler.getErrorByIndex(0);
```

## 6. 错误处理最佳实践

1. **及时报告错误**：在检测到错误时立即报告，不要忽略错误。
2. **提供详细信息**：在错误消息中包含足够的上下文信息，便于调试。
3. **合理分类严重级别**：根据错误的影响程度选择合适的严重级别。
4. **避免重复报告**：同一个错误不要重复报告多次。
5. **定期清理错误**：在适当时机清理错误历史，避免内存占用过多。

## 7. 测试

错误处理框架包含完整的测试用例，可以通过测试运行器运行：
```
15 - 错误处理框架测试
```

测试内容包括：
- 基本功能测试
- 错误历史记录功能测试
- 错误历史记录循环功能测试
- 清除错误功能测试

## 8. 扩展

如需添加新的错误代码：
1. 在 `ErrorCode` 枚举中添加新的错误代码
2. 在 `ErrorHandler::getErrorCodeString()` 方法中添加对应的描述
3. 在 `ErrorHandler::getDefaultSeverity()` 方法中设置默认严重级别（如果需要）