#include <Arduino.h>
#include "event_system.h"

// 简单的测试框架
class EventSystemTest {
private:
    int testsPassed;
    int testsTotal;
    
public:
    EventSystemTest() : testsPassed(0), testsTotal(0) {}
    
    void runAllTests() {
        DEBUG_INFO_PRINT("=== EventSystem Test Suite ===");
        
        // 运行测试
        testInitialization();
        testEventPublishing();
        testEventListeners();
        testOneTimeListeners();
        testRemoveListeners();
        testEventProcessing();
        
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
        
        EventSystem* eventSystem = EventSystem::getInstance();
        assertTrue(eventSystem != nullptr, "EventSystem instance creation");
        
        bool initResult = eventSystem->initialize();
        assertTrue(initResult, "EventSystem initialization");
        assertTrue(eventSystem->isInitialized(), "EventSystem is initialized");
        
        // 清理
        eventSystem->cleanup();
    }
    
    void testEventPublishing() {
        DEBUG_INFO_PRINT("--- Testing Event Publishing ---");
        
        EventSystem* eventSystem = EventSystem::getInstance();
        eventSystem->initialize();
        
        // 测试发布事件
        bool publishResult = eventSystem->publishEvent(EVENT_SYSTEM_STARTUP, "TestSource", "TestMessage");
        assertTrue(publishResult, "Event publishing successful");
        assertTrue(eventSystem->getQueueSize() == 1, "Event queue size is 1");
        
        // 测试获取事件类型名称
        String typeName = eventSystem->getEventTypeName(EVENT_SYSTEM_STARTUP);
        assertTrue(typeName == "SYSTEM_STARTUP", "Event type name conversion");
        
        // 测试自定义事件类型名称
        String customTypeName = eventSystem->getEventTypeName(static_cast<EventType>(EVENT_CUSTOM_BASE + 1));
        assertTrue(customTypeName == "CUSTOM_1", "Custom event type name conversion");
        
        // 清理
        eventSystem->cleanup();
    }
    
    void testEventListeners() {
        DEBUG_INFO_PRINT("--- Testing Event Listeners ---");
        
        EventSystem* eventSystem = EventSystem::getInstance();
        eventSystem->initialize();
        
        // 添加监听器
        bool addResult = eventSystem->addEventListener(EVENT_SYSTEM_STARTUP, "TestListener", 
            [](const EventData& event) {
                DEBUG_INFO_PRINT("Event received: %s", event.message.c_str());
            });
        assertTrue(addResult, "Adding event listener successful");
        assertTrue(eventSystem->getListenerCount(EVENT_SYSTEM_STARTUP) == 1, "Listener count is 1");
        assertTrue(eventSystem->getTotalListenerCount() == 1, "Total listener count is 1");
        
        // 尝试添加同名监听器（应该失败）
        bool addDuplicateResult = eventSystem->addEventListener(EVENT_SYSTEM_STARTUP, "TestListener", 
            [](const EventData& event) {
                DEBUG_INFO_PRINT("This should not be called");
            });
        assertTrue(!addDuplicateResult, "Adding duplicate listener should fail");
        
        // 添加另一个监听器
        bool addSecondResult = eventSystem->addEventListener(EVENT_WIFI_CONNECTED, "SecondListener", 
            [](const EventData& event) {
                DEBUG_INFO_PRINT("WiFi event received: %s", event.message.c_str());
            });
        assertTrue(addSecondResult, "Adding second listener successful");
        assertTrue(eventSystem->getListenerCount(EVENT_WIFI_CONNECTED) == 1, "WiFi listener count is 1");
        assertTrue(eventSystem->getTotalListenerCount() == 2, "Total listener count is 2");
        
        // 清理
        eventSystem->cleanup();
    }
    
    void testOneTimeListeners() {
        DEBUG_INFO_PRINT("--- Testing One-Time Listeners ---");
        
        EventSystem* eventSystem = EventSystem::getInstance();
        eventSystem->initialize();
        
        // 添加一次性监听器
        bool addResult = eventSystem->addEventListener(EVENT_SYSTEM_STARTUP, "OneTimeListener", 
            [](const EventData& event) {
                DEBUG_INFO_PRINT("One-time event received: %s", event.message.c_str());
            }, true); // oneTime = true
        assertTrue(addResult, "Adding one-time listener successful");
        assertTrue(eventSystem->getListenerCount(EVENT_SYSTEM_STARTUP) == 1, "One-time listener count is 1");
        
        // 发布事件
        eventSystem->publishEvent(EVENT_SYSTEM_STARTUP, "TestSource", "TestMessage");
        assertTrue(eventSystem->getQueueSize() == 1, "Event queue size is 1");
        
        // 处理事件
        eventSystem->processEvents();
        
        // 检查监听器是否已被移除
        assertTrue(eventSystem->getListenerCount(EVENT_SYSTEM_STARTUP) == 0, "One-time listener should be removed after processing");
        
        // 清理
        eventSystem->cleanup();
    }
    
    void testRemoveListeners() {
        DEBUG_INFO_PRINT("--- Testing Listener Removal ---");
        
        EventSystem* eventSystem = EventSystem::getInstance();
        eventSystem->initialize();
        
        // 添加监听器
        eventSystem->addEventListener(EVENT_SYSTEM_STARTUP, "Listener1", 
            [](const EventData& event) {});
        eventSystem->addEventListener(EVENT_SYSTEM_STARTUP, "Listener2", 
            [](const EventData& event) {});
        eventSystem->addEventListener(EVENT_WIFI_CONNECTED, "Listener3", 
            [](const EventData& event) {});
        
        assertTrue(eventSystem->getTotalListenerCount() == 3, "Total listener count is 3");
        
        // 移除特定监听器
        bool removeResult = eventSystem->removeEventListener(EVENT_SYSTEM_STARTUP, "Listener1");
        assertTrue(removeResult, "Removing specific listener successful");
        assertTrue(eventSystem->getTotalListenerCount() == 2, "Total listener count is 2 after removal");
        
        // 尝试移除不存在的监听器
        bool removeNonExistentResult = eventSystem->removeEventListener(EVENT_SYSTEM_STARTUP, "NonExistent");
        assertTrue(!removeNonExistentResult, "Removing non-existent listener should fail");
        
        // 移除特定事件类型的所有监听器
        eventSystem->removeAllListeners(EVENT_SYSTEM_STARTUP);
        assertTrue(eventSystem->getListenerCount(EVENT_SYSTEM_STARTUP) == 0, "All listeners for event type removed");
        assertTrue(eventSystem->getTotalListenerCount() == 1, "Total listener count is 1 after removing event type listeners");
        
        // 移除所有监听器
        eventSystem->removeAllListeners();
        assertTrue(eventSystem->getTotalListenerCount() == 0, "All listeners removed");
        
        // 清理
        eventSystem->cleanup();
    }
    
    void testEventProcessing() {
        DEBUG_INFO_PRINT("--- Testing Event Processing ---");
        
        EventSystem* eventSystem = EventSystem::getInstance();
        eventSystem->initialize();
        
        bool eventReceived = false;
        
        // 添加监听器
        eventSystem->addEventListener(EVENT_SYSTEM_STARTUP, "ProcessingTestListener", 
            [&eventReceived](const EventData& event) {
                DEBUG_INFO_PRINT("Processing test event received: %s", event.message.c_str());
                eventReceived = true;
            });
        
        // 发布事件
        eventSystem->publishEvent(EVENT_SYSTEM_STARTUP, "TestSource", "ProcessEvent");
        assertTrue(eventSystem->getQueueSize() == 1, "Event queue size is 1");
        
        // 处理事件
        eventSystem->processEvents();
        assertTrue(eventReceived, "Event should be processed");
        assertTrue(eventSystem->getQueueSize() == 0, "Event queue should be empty after processing");
        
        // 清理
        eventSystem->cleanup();
    }
};

// 全局测试实例
EventSystemTest* g_test = nullptr;

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    DEBUG_INFO_PRINT("Starting EventSystem Tests...");
    
    g_test = new EventSystemTest();
    g_test->runAllTests();
    
    DEBUG_INFO_PRINT("Tests completed. System will now halt.");
}

void loop() {
    // 测试完成后停止
    delay(1000);
}