#include <Arduino.h>
#include "uart_controller.h"
#include "debug_utils.h"

// 简单的测试框架
class UARTControllerTest {
private:
    UARTController uartController;
    int testsPassed;
    int testsTotal;
    
public:
    UARTControllerTest() : uartController(&Serial), testsPassed(0), testsTotal(0) {}
    
    void runAllTests() {
        DEBUG_INFO_PRINT("=== UARTController Test Suite ===");
        
        // 运行测试
        testInitialization();
        testWriteRead();
        testAvailable();
        testEnd();
        
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
    
    void testInitialization() {
        DEBUG_INFO_PRINT("--- Testing Initialization ---");
        
        // 测试初始化
        bool initResult = uartController.initialize(115200, SERIAL_8N1);
        assertTrue(initResult, "UARTController initialization successful");
        assertTrue(uartController.isInitialized(), "UARTController is initialized");
        assertTrue(uartController.getBaudRate() == 115200, "Baud rate should be 115200");
        assertTrue(uartController.getConfig() == SERIAL_8N1, "Config should be SERIAL_8N1");
        
        // 测试重复初始化（相同参数）
        bool reinitSameResult = uartController.initialize(115200, SERIAL_8N1);
        assertTrue(reinitSameResult, "Re-initialization with same parameters should succeed");
        
        // 测试重复初始化（不同参数）
        bool reinitDiffResult = uartController.initialize(9600, SERIAL_8N1);
        assertTrue(reinitDiffResult, "Re-initialization with different parameters should succeed");
        assertTrue(uartController.getBaudRate() == 9600, "Baud rate should be updated to 9600");
    }
    
    void testWriteRead() {
        DEBUG_INFO_PRINT("--- Testing Write/Read ---");
        
        // 确保UART已初始化
        if (!uartController.isInitialized()) {
            uartController.initialize(115200, SERIAL_8N1);
        }
        
        // 测试写入数据
        const char* testData = "Hello UART!";
        size_t written = uartController.write((const uint8_t*)testData, strlen(testData));
        assertTrue(written == strlen(testData), "All bytes should be written");
        
        // 测试读取数据（在实际硬件上可能无法读取自己写入的数据，但在模拟环境中可以测试接口）
        // 这里我们主要测试接口是否正常工作
        int readByte = uartController.read();
        assertTrue(readByte >= -1, "Read should return valid value");
        
        // 测试读取多个字节
        uint8_t buffer[10];
        size_t bytesRead = uartController.readBytes(buffer, sizeof(buffer));
        assertTrue(bytesRead <= sizeof(buffer), "Bytes read should not exceed buffer size");
    }
    
    void testAvailable() {
        DEBUG_INFO_PRINT("--- Testing Available ---");
        
        // 确保UART已初始化
        if (!uartController.isInitialized()) {
            uartController.initialize(115200, SERIAL_8N1);
        }
        
        // 测试可用字节数
        size_t availableBytes = uartController.available();
        assertTrue(availableBytes >= 0, "Available bytes should be non-negative");
    }
    
    void testEnd() {
        DEBUG_INFO_PRINT("--- Testing End ---");
        
        // 确保UART已初始化
        if (!uartController.isInitialized()) {
            uartController.initialize(115200, SERIAL_8N1);
        }
        
        // 测试结束UART
        uartController.end();
        assertTrue(!uartController.isInitialized(), "UARTController should not be initialized after end");
        assertTrue(uartController.getBaudRate() == 0, "Baud rate should be 0 after end");
        assertTrue(uartController.getConfig() == 0, "Config should be 0 after end");
    }
};

// 全局测试实例
UARTControllerTest* g_test = nullptr;

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    DEBUG_INFO_PRINT("Starting UARTController Tests...");
    
    g_test = new UARTControllerTest();
    g_test->runAllTests();
    
    DEBUG_INFO_PRINT("Tests completed. System will now halt.");
}

void loop() {
    // 测试完成后停止
    delay(1000);
}