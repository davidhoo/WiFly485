#include "config_manager.h"
#include <Arduino.h>

// 测试结果统计
static int test_passed = 0;
static int test_failed = 0;

// 测试函数声明
void test_config_manager_initialization();
void test_default_config_generation();
void test_config_validation();
void test_config_file_operations();
void test_config_getters_setters();

// 辅助函数
void print_test_result(const char* test_name, bool passed);

void run_config_tests() {
  Serial.println("Running Config Manager Tests...");
  Serial.println("==============================");
  
  test_config_manager_initialization();
  test_default_config_generation();
  test_config_validation();
  test_config_file_operations();
  test_config_getters_setters();
  
  Serial.println("==============================");
  Serial.printf("Tests passed: %d\n", test_passed);
  Serial.printf("Tests failed: %d\n", test_failed);
  Serial.println("==============================");
}

void test_config_manager_initialization() {
  Serial.println("Test: Config Manager Initialization");
  
  ConfigManager configManager;
  bool result = configManager.begin();
  
  print_test_result("ConfigManager::begin()", result);
}

void test_default_config_generation() {
  Serial.println("Test: Default Config Generation");
  
  ConfigManager configManager;
  configManager.generateDefaultConfig();
  
  // 检查默认配置是否生成
  NetworkConfig networkConfig = configManager.getNetworkConfig();
  RS485Config rs485Config = configManager.getRS485Config();
  DeviceConfig deviceConfig = configManager.getDeviceConfig();
  
  bool result = (networkConfig.ssid.length() > 0) &&
                (rs485Config.baudRate > 0) &&
                (deviceConfig.name.length() > 0);
  
  print_test_result("Default config generation", result);
}

void test_config_validation() {
  Serial.println("Test: Config Validation");
  
  ConfigManager configManager;
  configManager.generateDefaultConfig();
  
  // 验证默认配置
  bool result = configManager.validateConfig();
  
  print_test_result("Default config validation", result);
  
  // 测试无效配置
  NetworkConfig invalidNetworkConfig;
  invalidNetworkConfig.ssid = "";  // 无效SSID
  invalidNetworkConfig.password = "test_password";
  invalidNetworkConfig.dhcpEnabled = true;
  invalidNetworkConfig.ip = "192.168.1.100";
  invalidNetworkConfig.gateway = "192.168.1.1";
  invalidNetworkConfig.subnet = "255.255.255.0";
  
  configManager.setNetworkConfig(invalidNetworkConfig);
  bool invalidResult = !configManager.validateConfig();  // 应该返回false
  
  print_test_result("Invalid config validation", invalidResult);
}

void test_config_file_operations() {
  Serial.println("Test: Config File Operations");
  
  ConfigManager configManager;
  configManager.generateDefaultConfig();
  
  // 保存配置
  bool saveResult = configManager.saveConfig();
  print_test_result("Config save", saveResult);
  
  // 检查文件是否存在
  bool existsResult = configManager.configFileExists();
  print_test_result("Config file exists", existsResult);
  
  // 重新加载配置
  ConfigManager configManager2;
  bool loadResult = configManager2.loadConfig();
  print_test_result("Config load", loadResult);
  
  // 删除配置文件
  bool deleteResult = configManager.deleteConfigFile();
  print_test_result("Config file delete", deleteResult);
}

void test_config_getters_setters() {
  Serial.println("Test: Config Getters and Setters");
  
  ConfigManager configManager;
  configManager.generateDefaultConfig();
  
  // 获取原始配置
  NetworkConfig originalNetwork = configManager.getNetworkConfig();
  
  // 修改配置
  NetworkConfig newNetworkConfig;
  newNetworkConfig.ssid = "TestNetwork";
  newNetworkConfig.password = "Testpassword";
  newNetworkConfig.dhcpEnabled = false;
  newNetworkConfig.ip = "192.168.1.100";
  newNetworkConfig.gateway = "192.168.1.1";
  newNetworkConfig.subnet = "255.255.255.0";
  
  configManager.setNetworkConfig(newNetworkConfig);
  
  // 获取修改后的配置
  NetworkConfig modifiedNetwork = configManager.getNetworkConfig();
  
  // 验证配置是否正确修改
  bool result = (modifiedNetwork.ssid == "TestNetwork") &&
                (modifiedNetwork.password == "Testpassword") &&
                (modifiedNetwork.dhcpEnabled == false) &&
                (modifiedNetwork.ip == "192.168.1.100");
  
  print_test_result("Config getters and setters", result);
}

void print_test_result(const char* test_name, bool passed) {
  if (passed) {
    Serial.printf("  [PASS] %s\n", test_name);
    test_passed++;
  } else {
    Serial.printf("  [FAIL] %s\n", test_name);
    test_failed++;
  }
}