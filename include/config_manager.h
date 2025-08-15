#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <FS.h>

// 配置结构体定义
struct NetworkConfig {
  String ssid;
  String password;
  bool dhcpEnabled;
  String ip;
  String gateway;
  String subnet;
};

struct RS485Config {
  uint32_t baudRate;
  uint8_t dataBits;
  uint8_t parity;
  uint8_t stopBits;
};

struct DeviceConfig {
  String name;
  String role;  // "master" or "slave"
  uint16_t tcpPort;
  uint16_t syncPort;
};

class ConfigManager {
public:
  ConfigManager();
  ~ConfigManager();

  // 初始化配置管理器
  bool begin();
  
  // 加载配置
  bool loadConfig();
  
  // 保存配置
  bool saveConfig();
  
  // 生成默认配置
  void generateDefaultConfig();
  
  // 验证配置
  bool validateConfig();
  
  // 获取配置
  NetworkConfig getNetworkConfig();
  RS485Config getRS485Config();
  DeviceConfig getDeviceConfig();
  
  // 设置配置
  void setNetworkConfig(const NetworkConfig& config);
  void setRS485Config(const RS485Config& config);
  void setDeviceConfig(const DeviceConfig& config);
  
  // 检查配置文件是否存在
  bool configFileExists();
  
  // 删除配置文件
  bool deleteConfigFile();

private:
  // 配置数据
  NetworkConfig networkConfig;
  RS485Config rs485Config;
  DeviceConfig deviceConfig;
  
  // 配置文件路径
  const char* CONFIG_FILE_PATH = "/config.json";
  
  // SPIFFS相关操作
  bool mountSPIFFS();
  void unmountSPIFFS();
  
  // 内部辅助函数
  bool parseConfigFile();
  bool writeConfigFile();
};

#endif // CONFIG_MANAGER_H