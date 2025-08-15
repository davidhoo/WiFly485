#include <Arduino.h>
#include "debug_utils.h"

// 简单的测试框架
class DebugUtilsTest {
private:
    int testsPassed;
    int testsTotal;
    
public:
    DebugUtilsTest() : testsPassed(0), testsTotal(0) {}
    
    void runAllTests() {
        DEBUG_INFO_PRINT("=== DebugUtils Test Suite ===");
        
        // 运行测试
        testDebugLevelString();
        testPrintFunctions();
        
        // 输出结果
        DEBUG_INFO_PRINT("=== Test Results ===");
        DEBUG_INFO_PRINT("Tests passed: %d/%d", testsPassed, testsTotal);
        if (testsPassed == testsTotal) {
            DEBUG_INFO_PRINT("All tests PASSED!");
        } else {
            DEBUG_ERROR_PRINT("Some tests FAILED!");
        }
    }
    
private:
    void assertTrue(bool condition, const String& testName) {
        testsTotal++;
        if (condition) {
            testsPassed++;
            DEBUG_INFO_PRINT("✓ %s", testName.c_str());
        } else {
            DEBUG_ERROR_PRINT("✗ %s", testName.c_str());
        }
    }
    
    void testDebugLevelString() {
        DEBUG_INFO_PRINT("--- Testing Debug Level String Conversion ---");
        
        assertTrue(strcmp(getDebugLevelString(DEBUG_ERROR), "ERROR") == 0, "DEBUG_ERROR string conversion");
        assertTrue(strcmp(getDebugLevelString(DEBUG_WARNING), "WARN") == 0, "DEBUG_WARNING string conversion");
        assertTrue(strcmp(getDebugLevelString(DEBUG_INFO), "INFO") == 0, "DEBUG_INFO string conversion");
        assertTrue(strcmp(getDebugLevelString(DEBUG_VERBOSE), "VERB") == 0, "DEBUG_VERBOSE string conversion");
        assertTrue(strcmp(getDebugLevelString(DEBUG_NONE), "NONE") == 0, "DEBUG_NONE string conversion");
        assertTrue(strcmp(getDebugLevelString(static_cast<DebugLevel>(999)), "NONE") == 0, "Invalid debug level string conversion");
    }
    
    void testPrintFunctions() {
        DEBUG_INFO_PRINT("--- Testing Print Functions ---");
        
        // 测试系统信息打印函数
        DEBUG_INFO_PRINT("Testing printSystemInfo()");
        printSystemInfo();
        assertTrue(true, "printSystemInfo() executed without crash");
        
        // 测试内存信息打印函数
        DEBUG_INFO_PRINT("Testing printMemoryInfo()");
        printMemoryInfo();
        assertTrue(true, "printMemoryInfo() executed without crash");
    }
};

// 全局测试实例
DebugUtilsTest* g_test = nullptr;

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    DEBUG_INFO_PRINT("Starting DebugUtils Tests...");
    
    g_test = new DebugUtilsTest();
    g_test->runAllTests();
    
    DEBUG_INFO_PRINT("Tests completed. System will now halt.");
}

void loop() {
    // 测试完成后停止
    delay(1000);
}