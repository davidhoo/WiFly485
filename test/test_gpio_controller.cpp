#include <Arduino.h>
#include "gpio_controller.h"
#include "debug_utils.h"

// 简单的测试框架
class GPIOControllerTest {
private:
    GPIOController gpioController;
    int testsPassed;
    int testsTotal;
    
public:
    GPIOControllerTest() : testsPassed(0), testsTotal(0) {}
    
    void runAllTests() {
        DEBUG_INFO_PRINT("=== GPIOController Test Suite ===");
        
        // 初始化GPIO控制器
        if (!gpioController.initialize()) {
            DEBUG_ERROR_PRINT("Failed to initialize GPIOController for testing");
            return;
        }
        
        // 运行测试
        testInitialization();
        testPinMode();
        testDigitalWriteRead();
        testAnalogWriteRead();
        
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
        
        assertTrue(gpioController.isInitialized(), "GPIOController is initialized");
    }
    
    void testPinMode() {
        DEBUG_INFO_PRINT("--- Testing Pin Mode ---");
        
        // 测试设置引脚模式
        gpioController.pinMode(2, OUTPUT);
        assertTrue(true, "Setting pin mode to OUTPUT should not crash");
        
        gpioController.pinMode(2, INPUT);
        assertTrue(true, "Setting pin mode to INPUT should not crash");
        
        gpioController.pinMode(2, INPUT_PULLUP);
        assertTrue(true, "Setting pin mode to INPUT_PULLUP should not crash");
    }
    
    void testDigitalWriteRead() {
        DEBUG_INFO_PRINT("--- Testing Digital Write/Read ---");
        
        // 使用引脚2进行测试（通常是内置LED）
        uint8_t testPin = 2;
        
        // 设置引脚为输出模式
        gpioController.pinMode(testPin, OUTPUT);
        
        // 测试写入HIGH
        gpioController.digitalWrite(testPin, HIGH);
        assertTrue(true, "Writing HIGH to pin should not crash");
        
        // 测试写入LOW
        gpioController.digitalWrite(testPin, LOW);
        assertTrue(true, "Writing LOW to pin should not crash");
        
        // 设置引脚为输入模式
        gpioController.pinMode(testPin, INPUT);
        
        // 测试读取
        int value = gpioController.digitalRead(testPin);
        assertTrue(value == HIGH || value == LOW, "Reading from pin should return valid value");
    }
    
    void testAnalogWriteRead() {
        DEBUG_INFO_PRINT("--- Testing Analog Write/Read ---");
        
        // 使用引脚2进行测试（支持PWM输出）
        uint8_t testPin = 2;
        
        // 测试模拟写入
        gpioController.analogWrite(testPin, 0);
        assertTrue(true, "Analog write with value 0 should not crash");
        
        gpioController.analogWrite(testPin, 512);
        assertTrue(true, "Analog write with value 512 should not crash");
        
        gpioController.analogWrite(testPin, 1023);
        assertTrue(true, "Analog write with value 1023 should not crash");
        
        // 测试模拟读取（ESP8266的A0引脚）
        int analogValue = gpioController.analogRead(A0);
        assertTrue(analogValue >= 0 && analogValue <= 1023, "Analog read should return valid value");
    }
};

// 全局测试实例
GPIOControllerTest* g_test = nullptr;

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    DEBUG_INFO_PRINT("Starting GPIOController Tests...");
    
    g_test = new GPIOControllerTest();
    g_test->runAllTests();
    
    DEBUG_INFO_PRINT("Tests completed. System will now halt.");
}

void loop() {
    // 测试完成后停止
    delay(1000);
}