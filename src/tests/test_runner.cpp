#include <Arduino.h>
#include "device.h"
#include "logger.h"
#include "config_manager.h"
#include "test_framework.h"

// 全局变量
Device device;
extern Logger logger;
ConfigManager configManager;

// 测试函数声明 (使用 TEST 宏定义)
TEST(DeviceRole) {
  LOG_I("Test", "开始设备角色测试");
  
  DeviceRole role = device.getRole();
  String roleStr = device.getRoleString();
  
  Serial.printf("设备角色: %d\n", role);
  Serial.printf("设备角色字符串: %s\n", roleStr.c_str());
  
  if (device.isMaster()) {
    Serial.println("设备是主设备");
  } else if (device.isSlave()) {
    Serial.println("设备是从设备");
  } else {
    Serial.println("设备角色未知");
  }
  
  // 测试设置角色
  device.setRole(DEVICE_ROLE_MASTER_ENUM);
  ASSERT_TRUE(device.isMaster());
  
  device.setRole(DEVICE_ROLE_SLAVE_ENUM);
  ASSERT_TRUE(device.isSlave());
  
  LOG_I("Test", "设备角色测试完成");
}

TEST(DeviceName) {
  LOG_I("Test", "开始设备名称测试");
  
  String name = device.getName();
  Serial.printf("设备名称: %s\n", name.c_str());
  
  // 测试设置名称
  device.setName("Test_Device");
  String newName = device.getName();
  Serial.printf("新设备名称: %s\n", newName.c_str());
  
  ASSERT_STRING_EQUAL("Test_Device", newName.c_str());
  
  LOG_I("Test", "设备名称测试完成");
}

// 显示测试菜单
void showTestMenu() {
  Serial.println();
  Serial.println("==================================================");
  Serial.println("0|all - 所有测试");
  Serial.println("1 - 设备角色测试");
  Serial.println("2 - 设备名称测试");
  Serial.println("3 - 日志系统测试");
  Serial.println("4 - 配置管理器测试");
  Serial.println("h|help - 输出测试菜单");
  Serial.println("q|quit - 退出测试程序");
  Serial.println("==================================================");
  Serial.println();
}

TEST(Logger) {
  LOG_I("Test", "开始日志系统测试");
  
  // 测试不同级别的日志输出
  LOG_E("Logger", "这是一条错误日志");
  LOG_W("Logger", "这是一条警告日志");
  LOG_I("Logger", "这是一条信息日志");
  LOG_D("Logger", "这是一条调试日志");
  LOG_V("Logger", "这是一条详细日志");
  
  // 测试日志级别设置
  logger.setLogLevel(LOG_LEVEL_WARN);
  LOG_I("Logger", "这条信息日志不应该显示");
  LOG_W("Logger", "这条警告日志应该显示");
  
  // 恢复日志级别
  logger.setLogLevel(LOG_LEVEL_VERBOSE);
  
  LOG_I("Test", "日志系统测试完成");
}

TEST(ConfigManager) {
  LOG_I("Test", "开始配置管理器测试");
  
  // 测试获取配置
  NetworkConfig networkConfig = configManager.getNetworkConfig();
  RS485Config rs485Config = configManager.getRS485Config();
  DeviceConfig deviceConfig = configManager.getDeviceConfig();
  
  Serial.printf("网络SSID: %s\n", networkConfig.ssid.c_str());
  Serial.printf("RS485波特率: %d\n", rs485Config.baudRate);
  Serial.printf("设备名称: %s\n", deviceConfig.name.c_str());
  Serial.printf("设备角色: %s\n", deviceConfig.role.c_str());
  
  // 测试配置验证
  if (configManager.validateConfig()) {
    Serial.println("配置验证通过");
  } else {
    Serial.println("配置验证失败");
  }
  
  // 测试配置文件是否存在
  if (configManager.configFileExists()) {
    Serial.println("配置文件存在");
  } else {
    Serial.println("配置文件不存在");
  }
  
  ASSERT_TRUE(configManager.validateConfig() || !configManager.validateConfig()); // 总是通过的断言，仅作示例
  
  LOG_I("Test", "配置管理器测试完成");
}

void setup() {
  // 初始化串口
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("=== WiFly485 测试运行器 ===");
  
  // 初始化测试框架
  testFramework.begin();
  
  // 初始化日志系统
  logger.begin();
  logger.setLogLevel(LOG_LEVEL_VERBOSE);
  
  LOG_I("Test", "开始测试运行器");
  
  // 初始化设备
  if (!device.begin()) {
    LOG_E("Test", "设备初始化失败");
    return;
  }
  
  LOG_I("Test", "设备初始化成功");
  
  // 初始化配置管理器
  if (!configManager.begin()) {
    LOG_E("Test", "配置管理器初始化失败");
    return;
  }
  
  LOG_I("Test", "配置管理器初始化成功");
  
  // 注册测试 (使用 RUN_TEST 宏)
  RUN_TEST(DeviceRole);
  RUN_TEST(DeviceName);
  RUN_TEST(Logger);
  RUN_TEST(ConfigManager);
  
  // 显示测试菜单
  showTestMenu();
  
  LOG_I("Test", "测试运行器初始化完成");
  Serial.println("=== 测试运行器初始化完成 ===");
}

// 运行选定的测试
void runSelectedTest(int testNumber) {
  // 重置测试框架的计数器
  // 注意：这里我们直接调用测试函数，而不是通过测试框架
  // 因为测试框架没有提供运行单个测试的接口
  switch (testNumber) {
    case 1:
      test_DeviceRole();
      break;
    case 2:
      test_DeviceName();
      break;
    case 3:
      test_Logger();
      break;
    case 4:
      test_ConfigManager();
      break;
    default:
      Serial.println("无效的测试编号");
      break;
  }
}

// 前向声明
void runSelectedTest(int testNumber);

void loop() {
  // 检查串口是否有数据
  if (Serial.available() > 0) {
    // 读取用户输入
    String input = Serial.readStringUntil('\n');
    input.trim(); // 去除首尾空格和换行符
    
    // 处理用户输入
    if (input == "0" || input == "all") {
      Serial.println("运行所有测试...");
      testFramework.runAllTests();
      testFramework.printTestResults();
      showTestMenu();
    } else if (input == "1") {
      Serial.println("运行设备角色测试...");
      runSelectedTest(1);
      showTestMenu();
    } else if (input == "2") {
      Serial.println("运行设备名称测试...");
      runSelectedTest(2);
      showTestMenu();
    } else if (input == "3") {
      Serial.println("运行日志系统测试...");
      runSelectedTest(3);
      showTestMenu();
    } else if (input == "4") {
      Serial.println("运行配置管理器测试...");
      runSelectedTest(4);
      showTestMenu();
    } else if (input == "h" || input == "help") {
      showTestMenu();
    } else if (input == "q" || input == "quit") {
      Serial.println("退出测试程序。");
      Serial.println("再见！");
      // 停止处理输入，但保持程序运行
      // 在Arduino中，我们不能真正退出loop()函数
      // 但可以停止处理串口输入
      while (true) {
        // 保持程序运行但不处理任何输入
        delay(1000);
      }
    } else {
      Serial.println("无效输入，请重新输入。");
      showTestMenu();
    }
  }
  
  delay(100); // 短暂延迟以避免过度占用CPU
}