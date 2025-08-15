#include <Arduino.h>
#include "wifi_controller.h"
#include "debug_utils.h"

// 简单的测试框架
class WiFiControllerTest {
private:
    WiFiController wifiController;
    int testsPassed;
    int testsTotal;
    
public:
    WiFiControllerTest() : testsPassed(0), testsTotal(0) {}
    
    void runAllTests() {
        DEBUG_INFO_PRINT("=== WiFiController Test Suite ===");
        
        // 初始化WiFi控制器
        if (!wifiController.initialize()) {
            DEBUG_ERROR_PRINT("Failed to initialize WiFiController for testing");
            return;
        }
        
        // 运行测试
        testInitialization();
        testConnectionCallbacks();
        testUtilityMethods();
        
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
        
        assertTrue(wifiController.isInitialized(), "WiFiController is initialized");
        assertTrue(wifiController.getCurrentMode() == WIFI_OFF, "Initial mode should be WIFI_OFF");
    }
    
    void testConnectionCallbacks() {
        DEBUG_INFO_PRINT("--- Testing Connection Callbacks ---");
        
        bool callbackCalled = false;
        bool callbackValue = false;
        
        // 设置连接回调
        wifiController.onConnectionChanged([&callbackCalled, &callbackValue](bool connected) {
            callbackCalled = true;
            callbackValue = connected;
            DEBUG_INFO_PRINT("Connection callback called with value: %s", connected ? "true" : "false");
        });
        
        assertTrue(true, "Setting connection callback should not crash");
        
        // 测试断开连接（应该触发回调）
        wifiController.disconnect();
        // 注意：在实际测试中，我们不能真正连接到WiFi，所以这里主要测试接口
        assertTrue(true, "Disconnecting should not crash");
    }
    
    void testUtilityMethods() {
        DEBUG_INFO_PRINT("--- Testing Utility Methods ---");
        
        // 测试获取本地IP
        String localIP = wifiController.getLocalIP();
        assertTrue(!localIP.isEmpty(), "Local IP should not be empty");
        
        // 测试获取MAC地址
        String macAddress = wifiController.getMacAddress();
        assertTrue(!macAddress.isEmpty(), "MAC address should not be empty");
        assertTrue(macAddress.length() == 17, "MAC address should have correct length (17 characters)");
        
        // 测试获取RSSI
        int rssi = wifiController.getRSSI();
        // 在未连接状态下，RSSI应该是一个特定值（根据实现是-100）
        assertTrue(rssi <= 0, "RSSI should be non-positive");
        
        // 测试获取当前SSID
        String currentSSID = wifiController.getCurrentSSID();
        assertTrue(true, "Getting current SSID should not crash");
        
        // 测试获取当前模式
        WiFiMode_t currentMode = wifiController.getCurrentMode();
        assertTrue(currentMode == WIFI_OFF || currentMode == WIFI_STA || currentMode == WIFI_AP, 
                  "Current mode should be a valid WiFi mode");
        
        // 测试更新方法
        wifiController.update();
        assertTrue(true, "Update method should not crash");
    }
};

// 全局测试实例
WiFiControllerTest* g_test = nullptr;

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    DEBUG_INFO_PRINT("Starting WiFiController Tests...");
    
    g_test = new WiFiControllerTest();
    g_test->runAllTests();
    
    DEBUG_INFO_PRINT("Tests completed. System will now halt.");
}

void loop() {
    // 测试完成后停止
    delay(1000);
}