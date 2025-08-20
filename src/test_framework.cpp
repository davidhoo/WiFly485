#include "test_framework.h"
#include "logger.h"

// 全局测试框架实例
TestFramework testFramework;

TestFramework::TestFramework() : testList(nullptr), totalTests(0), passedTests(0), failedTests(0) {
  // 构造函数
}

TestFramework::~TestFramework() {
  // 析构函数
  // 清理测试列表
  TestItem* current = testList;
  while (current != nullptr) {
    TestItem* next = current->next;
    delete current;
    current = next;
  }
}

void TestFramework::begin() {
  // 初始化测试框架
  LOG_I("TestFramework", "=== WiFly485 测试框架初始化 ===");
  totalTests = 0;
  passedTests = 0;
  failedTests = 0;
}

void TestFramework::runAllTests() {
  // 运行所有测试
  LOG_I("TestFramework", "=== 开始运行所有测试 ===");
  
  TestItem* current = testList;
  while (current != nullptr) {
    LOG_I("TestFramework", "运行测试: %s", current->testName);
    LOG_I("TestFramework", "运行测试: %s", current->testName);
    
    // 运行测试函数
    current->testFunc();
    
    current = current->next;
  }
  
  LOG_I("TestFramework", "=== 测试运行完成 ===");
}

void TestFramework::registerTest(void (*testFunc)(), const char* testName) {
  // 注册测试函数
  TestItem* item = new TestItem();
  item->testFunc = testFunc;
  item->testName = testName;
  item->next = nullptr;
  
  addTestItem(item);
  totalTests++;
}

void TestFramework::assertTrue(bool condition, const char* testName, const char* message) {
  // 断言为真
  if (condition) {
    passedTests++;
    LOG_I("TestFramework", "测试 %s 通过: %s", testName, message);
  } else {
    failedTests++;
    LOG_E("TestFramework", "测试 %s 失败: %s", testName, message);
    LOG_E("TestFramework", "  失败: %s", message);
  }
}

void TestFramework::assertEquals(int expected, int actual, const char* testName, const char* message) {
  // 断言相等
  if (expected == actual) {
    passedTests++;
    LOG_I("TestFramework", "测试 %s 通过: %s (期望: %d, 实际: %d)", testName, message, expected, actual);
  } else {
    failedTests++;
    LOG_E("TestFramework", "测试 %s 失败: %s (期望: %d, 实际: %d)", testName, message, expected, actual);
    LOG_E("TestFramework", "  失败: %s (期望: %d, 实际: %d)", message, expected, actual);
  }
}

void TestFramework::assertStringEquals(const char* expected, const char* actual, const char* testName, const char* message) {
  // 断言字符串相等
  if (strcmp(expected, actual) == 0) {
    passedTests++;
    LOG_I("TestFramework", "测试 %s 通过: %s (期望: %s, 实际: %s)", testName, message, expected, actual);
  } else {
    failedTests++;
    LOG_E("TestFramework", "测试 %s 失败: %s (期望: %s, 实际: %s)", testName, message, expected, actual);
    LOG_E("TestFramework", "  失败: %s (期望: %s, 实际: %s)", message, expected, actual);
  }
}

void TestFramework::printTestResults() {
  // 打印测试结果
  LOG_I("TestFramework", "=== 测试结果 ===");
  LOG_I("TestFramework", "总测试数: %d", totalTests);
  LOG_I("TestFramework", "通过测试: %d", passedTests);
  LOG_I("TestFramework", "失败测试: %d", failedTests);
  
  if (failedTests == 0) {
    LOG_I("TestFramework", "所有测试通过!");
    LOG_I("TestFramework", "所有测试通过! 总计: %d", totalTests);
  } else {
    LOG_E("TestFramework", "有测试失败!");
    LOG_E("TestFramework", "测试失败! 通过: %d, 失败: %d", passedTests, failedTests);
  }
}

void TestFramework::addTestItem(TestItem* item) {
  // 添加测试项到列表
  if (testList == nullptr) {
    testList = item;
  } else {
    TestItem* current = testList;
    while (current->next != nullptr) {
      current = current->next;
    }
    current->next = item;
  }
}