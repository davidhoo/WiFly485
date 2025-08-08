#include <Arduino.h>
#include "config_manager.h"
#include "filesystem_controller.h"
#include "debug_utils.h"

// 简单的测试框架
class ConfigManagerTest {
private:
    FileSystemController fsController;
    ConfigManager* configManager;
    int testsPassed;
    int testsTotal;
    
public:
    ConfigManagerTest() : testsPassed(0), testsTotal(0) {
        configManager = new ConfigManager(&fsController);
    }
    
    ~ConfigManagerTest() {
        delete configManager;
    }
    
    void runAllTests() {
        DEBUG_INFO_PRINT("=== ConfigManager Test Suite ===");
        
        // 初始化文件系统
        if (!fsController.initialize()) {
            DEBUG_ERROR_PRINT("Failed to initialize filesystem for testing");
            return;
        }
        
        // 运行测试
        testInitialization();
        testDefaultConfig();
        testConfigSerialization();
        testConfigValidation();
        testConfigUpdate();
        
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
        
        bool initResult = configManager->initialize();
        assertTrue(initResult, "ConfigManager initialization");
        
        bool startResult = configManager->start();
        assertTrue(startResult, "ConfigManager start");
        
        assertTrue(configManager->isConfigLoaded(), "Config loaded after start");
    }
    
    void testDefaultConfig() {
        DEBUG_INFO_PRINT("--- Testing Default Configuration ---");
        
        const SystemConfig& config = configManager->getConfig();
        
        // 测试设备配置
        assertTrue(!config.device.device_id.isEmpty(), "Device ID not empty");
        assertTrue(!config.device.device_name.isEmpty(), "Device name not empty");
        
        // 测试网络配置
        assertTrue(config.network.ip_mode == "dhcp", "Default IP mode is DHCP");
        assertTrue(!config.network.hostname.isEmpty(), "Hostname not empty");
        
        // 测试RS485配置
        assertTrue(config.rs485.baud_rate == 9600, "Default baud rate is 9600");
        assertTrue(config.rs485.data_bits == 8, "Default data bits is 8");
        assertTrue(config.rs485.stop_bits == 1, "Default stop bits is 1");
        assertTrue(config.rs485.parity == "none", "Default parity is none");
        
        // 测试通信配置
        assertTrue(config.communication.data_port == 8888, "Default data port is 8888");
        assertTrue(config.communication.sync_port == 8889, "Default sync port is 8889");
    }
    
    void testConfigSerialization() {
        DEBUG_INFO_PRINT("--- Testing Configuration Serialization ---");
        
        // 导出配置
        String configJson;
        bool exportResult = configManager->exportConfig(configJson);
        assertTrue(exportResult, "Config export successful");
        assertTrue(!configJson.isEmpty(), "Exported JSON not empty");
        
        // 导入配置
        bool importResult = configManager->importConfig(configJson);
        assertTrue(importResult, "Config import successful");
        
        DEBUG_VERBOSE_PRINT("Exported config JSON: %s", configJson.c_str());
    }
    
    void testConfigValidation() {
        DEBUG_INFO_PRINT("--- Testing Configuration Validation ---");
        
        // 测试有效的RS485配置
        RS485Config validRS485;
        validRS485.baud_rate = 9600;
        validRS485.data_bits = 8;
        validRS485.stop_bits = 1;
        validRS485.parity = "none";
        validRS485.direction_pin = 2;
        
        bool validResult = configManager->updateRS485Config(validRS485);
        assertTrue(validResult, "Valid RS485 config accepted");
        
        // 测试无效的RS485配置
        RS485Config invalidRS485;
        invalidRS485.baud_rate = 999999; // 无效波特率
        invalidRS485.data_bits = 8;
        invalidRS485.stop_bits = 1;
        invalidRS485.parity = "none";
        invalidRS485.direction_pin = 2;
        
        bool invalidResult = configManager->updateRS485Config(invalidRS485);
        assertTrue(!invalidResult, "Invalid RS485 config rejected");
        
        // 测试有效的通信配置
        CommunicationConfig validComm;
        validComm.data_port = 8888;
        validComm.sync_port = 8889;
        validComm.auto_push = true;
        validComm.auto_sync = false;
        validComm.sync_interval = 300;
        
        bool validCommResult = configManager->updateCommunicationConfig(validComm);
        assertTrue(validCommResult, "Valid communication config accepted");
        
        // 测试无效的通信配置（相同端口）
        CommunicationConfig invalidComm;
        invalidComm.data_port = 8888;
        invalidComm.sync_port = 8888; // 与数据端口相同
        invalidComm.auto_push = true;
        invalidComm.auto_sync = false;
        invalidComm.sync_interval = 300;
        
        bool invalidCommResult = configManager->updateCommunicationConfig(invalidComm);
        assertTrue(!invalidCommResult, "Invalid communication config rejected");
    }
    
    void testConfigUpdate() {
        DEBUG_INFO_PRINT("--- Testing Configuration Updates ---");
        
        // 获取原始配置
        const SystemConfig& originalConfig = configManager->getConfig();
        String originalHostname = originalConfig.network.hostname;
        
        // 更新网络配置
        NetworkConfig newNetwork = originalConfig.network;
        newNetwork.ssid = "TestWiFi";
        newNetwork.password = "TestPassword";
        
        bool updateResult = configManager->updateNetworkConfig(newNetwork);
        assertTrue(updateResult, "Network config update successful");
        
        // 验证更新
        const SystemConfig& updatedConfig = configManager->getConfig();
        assertTrue(updatedConfig.network.ssid == "TestWiFi", "SSID updated correctly");
        assertTrue(updatedConfig.network.password == "TestPassword", "Password updated correctly");
        assertTrue(updatedConfig.network.hostname == originalHostname, "Hostname preserved");
        
        // 测试配置重置
        bool resetResult = configManager->resetToDefault();
        assertTrue(resetResult, "Config reset to default successful");
        
        const SystemConfig& resetConfig = configManager->getConfig();
        assertTrue(resetConfig.network.ssid.isEmpty(), "SSID reset to empty");
        assertTrue(resetConfig.network.password.isEmpty(), "Password reset to empty");
    }
};

// 全局测试实例
ConfigManagerTest* g_test = nullptr;

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    DEBUG_INFO_PRINT("Starting ConfigManager Tests...");
    
    g_test = new ConfigManagerTest();
    g_test->runAllTests();
    
    DEBUG_INFO_PRINT("Tests completed. System will now halt.");
}

void loop() {
    // 测试完成后停止
    delay(1000);
}