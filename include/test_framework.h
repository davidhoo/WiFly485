#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <Arduino.h>

// 测试框架类
class TestFramework {
public:
  TestFramework();
  ~TestFramework();

  // 初始化测试框架
  void begin();
  
  // 运行所有测试
  void runAllTests();
  
  // 注册测试函数
  void registerTest(void (*testFunc)(), const char* testName);
  
  // 断言函数
  void assertTrue(bool condition, const char* testName, const char* message);
  void assertEquals(int expected, int actual, const char* testName, const char* message);
  void assertStringEquals(const char* expected, const char* actual, const char* testName, const char* message);
  
  // 测试结果统计
  void printTestResults();

private:
  // 测试项结构
  struct TestItem {
    void (*testFunc)();
    const char* testName;
    TestItem* next;
  };
  
  TestItem* testList;
  int totalTests;
  int passedTests;
  int failedTests;
  
  // 添加测试项到列表
  void addTestItem(TestItem* item);
};

// 全局测试框架实例
extern TestFramework testFramework;

// 测试宏定义
#define TEST(name) void test_##name()
#define RUN_TEST(name) testFramework.registerTest(test_##name, #name)
#define ASSERT_TRUE(condition) testFramework.assertTrue(condition, __FUNCTION__, #condition)
#define ASSERT_EQUAL(expected, actual) testFramework.assertEquals(expected, actual, __FUNCTION__, #expected " == " #actual)
#define ASSERT_STRING_EQUAL(expected, actual) testFramework.assertStringEquals(expected, actual, __FUNCTION__, #expected " == " #actual)

#endif // TEST_FRAMEWORK_H