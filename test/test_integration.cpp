#include <Arduino.h>
#include "debug_utils.h"
#include "gpio_controller.h"
#include "uart_controller.h"
#include "wifi_controller.h"
#include "filesystem_controller.h"
#include "service_manager.h"
#include "event_system.h"
#include "config_manager.h"
#include "led_indicator.h"

// 简单的测试框架
class IntegrationTest {
private:
    int testsPassed;
    int testsTotal;
    
public:
    IntegrationTest() : testsPassed(0), testsTotal(0) {}
    
    void runAllTests() {
        DEBUG_INFO_PRINT("=== Integration Test Suite ===");
        
        // 运行测试
        testSystemInitialization();
        testModuleIntegration();
        testEventSystem();
        testServiceManagement();
        
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
    
    void testSystemInitialization() {
        DEBUG_INFO_PRINT("--- Testing System Initialization ---");
        
        // 初始化事件系统
        EventSystem* eventSystem = EventSystem::getInstance();
        bool eventInitResult = eventSystem->initialize();
        assertTrue(eventInitResult, "Event system initialization");
        assertTrue(eventSystem->isInitialized(), "Event system is initialized");
        
        // 初始化文件系统控制器
        FileSystemController fsController;
        bool fsInitResult = fsController.initialize();
        assertTrue(fsInitResult, "File system controller initialization");
        assertTrue(fsController.isInitialized(), "File system controller is initialized");
        
        // 初始化服务管理器
        ServiceManager serviceManager;
        bool serviceManagerInitResult = serviceManager.initialize();
        assertTrue(serviceManagerInitResult, "Service manager initialization");
        assertTrue(serviceManager.isInitialized(), "Service manager is initialized");
    }
    
    void testModuleIntegration() {
        DEBUG_INFO_PRINT("--- Testing Module Integration ---");
        
        // 初始化硬件抽象层控制器
        GPIOController gpioController;
        UARTController uartController;
        WiFiController wifiController;
        FileSystemController fsController;
        
        bool gpioInitResult = gpioController.initialize();
        assertTrue(gpioInitResult, "GPIO controller initialization");
        assertTrue(gpioController.isInitialized(), "GPIO controller is initialized");
        
        bool uartInitResult = uartController.initialize(9600, SERIAL_8N1);
        assertTrue(uartInitResult, "UART controller initialization");
        assertTrue(uartController.isInitialized(), "UART controller is initialized");
        
        bool wifiInitResult = wifiController.initialize();
        assertTrue(wifiInitResult, "WiFi controller initialization");
        
        bool fsInitResult = fsController.initialize();
        assertTrue(fsInitResult, "File system controller initialization");
        assertTrue(fsController.isInitialized(), "File system controller is initialized");
        
        // 初始化LED指示器
        LEDIndicator ledIndicator(&gpioController);
        bool ledInitResult = ledIndicator.initialize();
        assertTrue(ledInitResult, "LED indicator initialization");
        assertTrue(ledIndicator.isInitialized(), "LED indicator is initialized");
        
        // 测试LED指示器功能
        ledIndicator.turnOn();
        assertTrue(ledIndicator.getState() == LED_STATE_ON, "LED state should be ON");
        
        ledIndicator.blinkFast();
        assertTrue(ledIndicator.getState() == LED_STATE_BLINK_FAST, "LED state should be BLINK_FAST");
        
        ledIndicator.turnOff();
        assertTrue(ledIndicator.getState() == LED_STATE_OFF, "LED state should be OFF");
    }
    
    void testEventSystem() {
        DEBUG_INFO_PRINT("--- Testing Event System ---");
        
        EventSystem* eventSystem = EventSystem::getInstance();
        
        // 测试事件发布和监听
        bool eventReceived = false;
        String receivedMessage = "";
        
        bool listenResult = eventSystem->addEventListener(EVENT_STATUS_UPDATE, "IntegrationTest", 
            [&eventReceived, &receivedMessage](const EventData& event) {
                eventReceived = true;
                receivedMessage = event.message;
                DEBUG_INFO_PRINT("Event received: %s", event.message.c_str());
            });
        assertTrue(listenResult, "Adding event listener");
        
        // 发布事件
        bool publishResult = eventSystem->publishEvent(EVENT_STATUS_UPDATE, "TestSource", "TestMessage");
        assertTrue(publishResult, "Publishing event");
        assertTrue(eventSystem->getQueueSize() == 1, "Event queue size should be 1");
        
        // 处理事件
        eventSystem->processEvents();
        assertTrue(eventReceived, "Event should be received");
        assertTrue(receivedMessage == "TestMessage", "Received message should match");
        
        // 检查队列是否已清空
        assertTrue(eventSystem->getQueueSize() == 0, "Event queue should be empty after processing");
    }
    
    void testServiceManagement() {
        DEBUG_INFO_PRINT("--- Testing Service Management ---");
        
        // 初始化服务管理器和相关组件
        ServiceManager serviceManager;
        FileSystemController fsController;
        
        serviceManager.initialize();
        fsController.initialize();
        
        // 创建配置管理器服务
        ConfigManager configManager(&fsController);
        
        // 注册服务
        bool registerResult = serviceManager.registerService(&configManager);
        assertTrue(registerResult, "Registering ConfigManager service");
        assertTrue(serviceManager.getServiceCount() == 1, "Service count should be 1");
        
        // 启动所有服务
        bool startAllResult = serviceManager.startAllServices();
        assertTrue(startAllResult, "Starting all services");
        assertTrue(configManager.getStatus() == SERVICE_RUNNING, "ConfigManager should be running");
        
        // 停止所有服务
        bool stopAllResult = serviceManager.stopAllServices();
        assertTrue(stopAllResult, "Stopping all services");
        assertTrue(configManager.getStatus() == SERVICE_STOPPED, "ConfigManager should be stopped");
    }
};

// 全局测试实例
IntegrationTest* g_test = nullptr;

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    DEBUG_INFO_PRINT("Starting Integration Tests...");
    
    g_test = new IntegrationTest();
    g_test->runAllTests();
    
    DEBUG_INFO_PRINT("Integration tests completed. System will now halt.");
}

void loop() {
    // 测试完成后停止
    delay(1000);
}