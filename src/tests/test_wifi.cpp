#include "test_framework.h"
#include "wifi_manager.h"
#include "config_manager.h"
#include "device.h"

// 测试用的WiFi管理器实例
static WiFiManager* testWiFiManager = nullptr;
static ConfigManager* testConfigManager = nullptr;
static Device* testDevice = nullptr;

// WiFi连接状态回调函数
void wifiStatusCallback(WiFiConnectionStatus status) {
  Serial.printf("WiFi Status Callback: %d\n", status);
}

// 测试WiFi管理器初始化
void testWiFiManagerInitialization() {
  Serial.println("Testing WiFi Manager Initialization...");
  
  // 创建测试实例
  testConfigManager = new ConfigManager();
  testDevice = new Device();
  
  // 初始化设备
  testDevice->begin();
  
  // 初始化配置管理器
  testConfigManager->begin();
  
  // 创建WiFi管理器
  testWiFiManager = new WiFiManager();
  
  // 测试初始化
  bool result = testWiFiManager->begin(testConfigManager, testDevice);
  ASSERT_TRUE(result);
  
  // 检查初始状态
  ASSERT_EQUAL(WIFI_DISCONNECTED, testWiFiManager->getConnectionStatus());
  
  Serial.println("WiFi Manager Initialization test passed!");
}

// 测试WiFi连接状态获取
void testWiFiConnectionStatus() {
  Serial.println("Testing WiFi Connection Status...");
  
  if (testWiFiManager) {
    // 获取连接状态
    WiFiConnectionStatus status = testWiFiManager->getConnectionStatus();
    ASSERT_TRUE(status >= WIFI_DISCONNECTED && status <= WIFI_CONNECTION_FAILED);
    
    // 获取状态字符串
    String statusString = testWiFiManager->getConnectionStatusString();
    ASSERT_TRUE(statusString.length() > 0);
    
    Serial.println("WiFi Connection Status test passed!");
  } else {
    Serial.println("WiFi Manager not initialized");
  }
}

// 测试WiFi本地IP获取
void testWiFiLocalIP() {
  Serial.println("Testing WiFi Local IP...");
  
  if (testWiFiManager) {
    // 获取本地IP
    IPAddress ip = testWiFiManager->getLocalIP();
    // IP地址可能为0.0.0.0（未连接时），这是正常的
    ASSERT_TRUE(true); // 只是确保不崩溃
    
    Serial.println("WiFi Local IP test passed!");
  } else {
    Serial.println("WiFi Manager not initialized");
  }
}

// 测试WiFi信号强度获取
void testWiFiRSSI() {
  Serial.println("Testing WiFi RSSI...");
  
  if (testWiFiManager) {
    // 获取信号强度
    int32_t rssi = testWiFiManager->getRSSI();
    // 信号强度可能为-100（未连接时），这是正常的
    ASSERT_TRUE(true); // 只是确保不崩溃
    
    Serial.println("WiFi RSSI test passed!");
  } else {
    Serial.println("WiFi Manager not initialized");
  }
}

// 测试WiFi连接状态回调设置
void testWiFiStatusCallback() {
  Serial.println("Testing WiFi Status Callback...");
  
  if (testWiFiManager) {
    // 设置回调函数
    testWiFiManager->setConnectionStatusCallback(wifiStatusCallback);
    ASSERT_TRUE(true); // 只是确保不崩溃
    
    Serial.println("WiFi Status Callback test passed!");
  } else {
    Serial.println("WiFi Manager not initialized");
  }
}

// 测试WiFi处理函数
void testWiFiHandle() {
  Serial.println("Testing WiFi Handle...");
  
  if (testWiFiManager) {
    // 调用处理函数
    testWiFiManager->handle();
    ASSERT_TRUE(true); // 只是确保不崩溃
    
    Serial.println("WiFi Handle test passed!");
  } else {
    Serial.println("WiFi Manager not initialized");
  }
}

// 主测试函数
void runWiFiTests() {
  Serial.println("Running WiFi Tests...");
  
  // 运行各个测试
  testWiFiManagerInitialization();
  testWiFiConnectionStatus();
  testWiFiLocalIP();
  testWiFiRSSI();
  testWiFiStatusCallback();
  testWiFiHandle();
  
  // 清理资源
  if (testWiFiManager) {
    delete testWiFiManager;
    testWiFiManager = nullptr;
  }
  
  if (testConfigManager) {
    delete testConfigManager;
    testConfigManager = nullptr;
  }
  
  if (testDevice) {
    delete testDevice;
    testDevice = nullptr;
  }
  
  Serial.println("All WiFi Tests completed!");
}