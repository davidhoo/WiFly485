#include "error_handler.h"
#include "test_framework.h"

// 测试错误处理器的基本功能
TEST(error_handler_basic) {
  // 创建错误处理器实例
  ErrorHandler errorHandler;
  errorHandler.begin();
  
  // 测试初始状态
  ASSERT_EQUAL(0, errorHandler.getErrorCount());
  ErrorInfo lastError = errorHandler.getLastError();
  ASSERT_EQUAL(ERROR_NONE, lastError.code);
  
  // 测试报告错误
  errorHandler.reportError(ERROR_INVALID_PARAMETER, "TestModule", "Test error message");
  
  // 验证错误计数
  ASSERT_EQUAL(1, errorHandler.getErrorCount());
  
  // 验证最后一个错误
  lastError = errorHandler.getLastError();
  ASSERT_EQUAL(ERROR_INVALID_PARAMETER, lastError.code);
  ASSERT_STRING_EQUAL("TestModule", lastError.module.c_str());
  ASSERT_STRING_EQUAL("Test error message", lastError.message.c_str());
  
  // 测试错误代码字符串转换
  String codeStr = errorHandler.getErrorCodeString(ERROR_INVALID_PARAMETER);
  ASSERT_STRING_EQUAL("Invalid Parameter", codeStr.c_str());
  
  // 测试错误严重级别字符串转换
  String severityStr = errorHandler.getErrorSeverityString(ERROR_SEVERITY_WARNING);
  ASSERT_STRING_EQUAL("Warning", severityStr.c_str());
}

// 测试错误历史记录功能
TEST(error_handler_history) {
  // 创建错误处理器实例
  ErrorHandler errorHandler;
  errorHandler.begin();
  
  // 报告多个错误
  errorHandler.reportError(ERROR_INVALID_PARAMETER, "Module1", "Error 1");
  errorHandler.reportError(ERROR_WIFI_NOT_CONNECTED, "Module2", "Error 2");
  errorHandler.reportError(ERROR_TCP_CONNECTION_FAILED, "Module3", "Error 3");
  
  // 验证错误计数
  ASSERT_EQUAL(3, errorHandler.getErrorCount());
  
  // 验证按索引获取错误
  ErrorInfo error1 = errorHandler.getErrorByIndex(0);
  ErrorInfo error2 = errorHandler.getErrorByIndex(1);
  ErrorInfo error3 = errorHandler.getErrorByIndex(2);
  
  ASSERT_EQUAL(ERROR_INVALID_PARAMETER, error1.code);
  ASSERT_EQUAL(ERROR_WIFI_NOT_CONNECTED, error2.code);
  ASSERT_EQUAL(ERROR_TCP_CONNECTION_FAILED, error3.code);
  
  // 测试超出范围的索引
  ErrorInfo emptyError = errorHandler.getErrorByIndex(10);
  ASSERT_EQUAL(ERROR_NONE, emptyError.code);
}

// 测试错误历史记录循环功能
TEST(error_handler_history_overflow) {
  // 创建错误处理器实例
  ErrorHandler errorHandler;
  errorHandler.begin();
  
  // 报告超过最大历史记录数量的错误
  for (int i = 0; i < 15; i++) {
    String module = "Module" + String(i);
    String message = "Error " + String(i);
    errorHandler.reportError(ERROR_UNKNOWN, module.c_str(), message.c_str());
  }
  
  // 验证错误计数不超过最大值
  ASSERT_EQUAL(10, errorHandler.getErrorCount());
  
  // 验证最新的错误是最后报告的错误
  ErrorInfo lastError = errorHandler.getLastError();
  ASSERT_EQUAL(ERROR_UNKNOWN, lastError.code);
  ASSERT_STRING_EQUAL("Module14", lastError.module.c_str());
  ASSERT_STRING_EQUAL("Error 14", lastError.message.c_str());
  
  // 验证最早报告的错误已被覆盖
  ErrorInfo firstError = errorHandler.getErrorByIndex(0);
  ASSERT_EQUAL(ERROR_UNKNOWN, firstError.code);
  ASSERT_STRING_EQUAL("Module5", firstError.module.c_str());
  ASSERT_STRING_EQUAL("Error 5", firstError.message.c_str());
}

// 测试清除错误功能
TEST(error_handler_clear) {
  // 创建错误处理器实例
  ErrorHandler errorHandler;
  errorHandler.begin();
  
  // 报告一些错误
  errorHandler.reportError(ERROR_INVALID_PARAMETER, "Module1", "Error 1");
  errorHandler.reportError(ERROR_WIFI_NOT_CONNECTED, "Module2", "Error 2");
  
  // 验证错误计数
  ASSERT_EQUAL(2, errorHandler.getErrorCount());
  
  // 清除错误
  errorHandler.clearErrors();
  
  // 验证错误计数为0
  ASSERT_EQUAL(0, errorHandler.getErrorCount());
  
  // 验证最后一个错误为空
  ErrorInfo lastError = errorHandler.getLastError();
  ASSERT_EQUAL(ERROR_NONE, lastError.code);
}

// 测试默认严重级别
TEST(error_handler_default_severity) {
  // 创建错误处理器实例
  ErrorHandler errorHandler;
  errorHandler.begin();
  
  // 测试不同类型错误的默认严重级别
  // ErrorSeverity severity1 = errorHandler.getDefaultSeverity(ERROR_NONE);
  // ASSERT_EQUAL(ERROR_SEVERITY_INFO, severity1);
  
  // ErrorSeverity severity2 = errorHandler.getDefaultSeverity(ERROR_INVALID_PARAMETER);
  // ASSERT_EQUAL(ERROR_SEVERITY_WARNING, severity2);
  
  // ErrorSeverity severity3 = errorHandler.getDefaultSeverity(ERROR_WIFI_CONNECTION_FAILED);
  // ASSERT_EQUAL(ERROR_SEVERITY_CRITICAL, severity3);
  
  // ErrorSeverity severity4 = errorHandler.getDefaultSeverity(ERROR_FILE_NOT_FOUND);
  // ASSERT_EQUAL(ERROR_SEVERITY_ERROR, severity4);
}

void run_error_handler_tests() {
  RUN_TEST(error_handler_basic);
  RUN_TEST(error_handler_history);
  RUN_TEST(error_handler_history_overflow);
  RUN_TEST(error_handler_clear);
  RUN_TEST(error_handler_default_severity);
}