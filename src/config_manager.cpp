#include "config_manager.h"
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <FS.h>

ConfigManager::ConfigManager() {
  // 初始化默认配置
  generateDefaultConfig();
}

ConfigManager::~ConfigManager() {
  // 析构函数
}

bool ConfigManager::begin() {
  // 挂载SPIFFS
  if (!mountSPIFFS()) {
    Serial.println("Failed to mount SPIFFS");
    return false;
  }
  
  // 如果配置文件存在，加载配置
  if (configFileExists()) {
    if (!loadConfig()) {
      Serial.println("Failed to load config, using default config");
      generateDefaultConfig();
    }
  } else {
    // 配置文件不存在，生成默认配置并保存
    Serial.println("Config file not found, generating default config");
    generateDefaultConfig();
    saveConfig();
  }
  
  // 验证配置
  if (!validateConfig()) {
    Serial.println("Invalid config, using default config");
    generateDefaultConfig();
  }
  
  return true;
}

bool ConfigManager::mountSPIFFS() {
  // 挂载SPIFFS文件系统
  if (!SPIFFS.begin()) {
    Serial.println("Failed to mount SPIFFS");
    return false;
  }
  return true;
}

void ConfigManager::unmountSPIFFS() {
  // 卸载SPIFFS文件系统
  SPIFFS.end();
}

bool ConfigManager::loadConfig() {
  // 检查配置文件是否存在
  if (!configFileExists()) {
    Serial.println("Config file does not exist");
    return false;
  }
  
  // 解析配置文件
  return parseConfigFile();
}

bool ConfigManager::saveConfig() {
  // 写入配置文件
  return writeConfigFile();
}

void ConfigManager::generateDefaultConfig() {
  // 生成网络配置默认值
  networkConfig.ssid = "WiFly485_Network";
  networkConfig.password = "default_password";
  networkConfig.dhcpEnabled = true;
  networkConfig.ip = "192.168.1.100";
  networkConfig.gateway = "192.168.1.1";
  networkConfig.subnet = "255.255.255.0";
  
  // 生成RS485配置默认值
  rs485Config.baudRate = 9600;
  rs485Config.dataBits = 8;
  rs485Config.parity = 0;  // 0: None, 1: Odd, 2: Even
  rs485Config.stopBits = 1;
  
  // 生成设备配置默认值
#ifdef DEVICE_ROLE_MASTER
  deviceConfig.name = "WiFly485_Master";
  deviceConfig.role = "master";
  deviceConfig.tcpPort = 8888;
  deviceConfig.syncPort = 8889;
#elif defined(DEVICE_ROLE_SLAVE)
  deviceConfig.name = "WiFly485_Slave";
  deviceConfig.role = "slave";
  deviceConfig.tcpPort = 0;  // Slave doesn't need TCP server
  deviceConfig.syncPort = 8889;
#else
  deviceConfig.name = "WiFly485_Device";
  deviceConfig.role = "unknown";
  deviceConfig.tcpPort = 8888;
  deviceConfig.syncPort = 8889;
#endif
}

bool ConfigManager::validateConfig() {
  // 验证网络配置
  if (networkConfig.ssid.length() == 0) {
    Serial.println("Invalid SSID");
    return false;
  }
  
  if (networkConfig.password.length() == 0) {
    Serial.println("Invalid password");
    return false;
  }
  
  if (!networkConfig.dhcpEnabled) {
    if (networkConfig.ip.length() == 0 || 
        networkConfig.gateway.length() == 0 || 
        networkConfig.subnet.length() == 0) {
      Serial.println("Invalid static IP configuration");
      return false;
    }
  }
  
  // 验证RS485配置
  if (rs485Config.baudRate < 1200 || rs485Config.baudRate > 115200) {
    Serial.println("Invalid baud rate");
    return false;
  }
  
  if (rs485Config.dataBits < 5 || rs485Config.dataBits > 8) {
    Serial.println("Invalid data bits");
    return false;
  }
  
  if (rs485Config.parity < 0 || rs485Config.parity > 2) {
    Serial.println("Invalid parity");
    return false;
  }
  
  if (rs485Config.stopBits < 1 || rs485Config.stopBits > 2) {
    Serial.println("Invalid stop bits");
    return false;
  }
  
  // 验证设备配置
  if (deviceConfig.name.length() == 0) {
    Serial.println("Invalid device name");
    return false;
  }
  
  if (deviceConfig.role != "master" && deviceConfig.role != "slave") {
    Serial.println("Invalid device role");
    return false;
  }
  
  if (deviceConfig.tcpPort < 0 || deviceConfig.tcpPort > 65535) {
    Serial.println("Invalid TCP port");
    return false;
  }
  
  if (deviceConfig.syncPort < 0 || deviceConfig.syncPort > 65535) {
    Serial.println("Invalid sync port");
    return false;
  }
  
  return true;
}

NetworkConfig ConfigManager::getNetworkConfig() {
  return networkConfig;
}

RS485Config ConfigManager::getRS485Config() {
  return rs485Config;
}

DeviceConfig ConfigManager::getDeviceConfig() {
  return deviceConfig;
}

void ConfigManager::setNetworkConfig(const NetworkConfig& config) {
  networkConfig = config;
}

void ConfigManager::setRS485Config(const RS485Config& config) {
  rs485Config = config;
}

void ConfigManager::setDeviceConfig(const DeviceConfig& config) {
  deviceConfig = config;
}

bool ConfigManager::configFileExists() {
  return SPIFFS.exists(CONFIG_FILE_PATH);
}

bool ConfigManager::deleteConfigFile() {
  return SPIFFS.remove(CONFIG_FILE_PATH);
}

bool ConfigManager::parseConfigFile() {
  // 打开配置文件
  File configFile = SPIFFS.open(CONFIG_FILE_PATH, "r");
  if (!configFile) {
    Serial.println("Failed to open config file for reading");
    return false;
  }
  
  // 获取文件大小
  size_t size = configFile.size();
  if (size > 4096) {
    Serial.println("Config file size is too large");
    configFile.close();
    return false;
  }
  
  // 分配缓冲区并读取文件内容
  std::unique_ptr<char[]> buf(new char[size]);
  configFile.readBytes(buf.get(), size);
  configFile.close();
  
  // 解析JSON
  DynamicJsonDocument doc(4096);
  DeserializationError error = deserializeJson(doc, buf.get());
  if (error) {
    Serial.println("Failed to parse config file");
    return false;
  }
  
  // 解析网络配置
  JsonObject network = doc["network"];
  networkConfig.ssid = network["ssid"].as<String>();
  networkConfig.password = network["password"].as<String>();
  networkConfig.dhcpEnabled = network["dhcpEnabled"];
  networkConfig.ip = network["ip"].as<String>();
  networkConfig.gateway = network["gateway"].as<String>();
  networkConfig.subnet = network["subnet"].as<String>();
  
  // 解析RS485配置
  JsonObject rs485 = doc["rs485"];
  rs485Config.baudRate = rs485["baudRate"];
  rs485Config.dataBits = rs485["dataBits"];
  rs485Config.parity = rs485["parity"];
  rs485Config.stopBits = rs485["stopBits"];
  
  // 解析设备配置
  JsonObject device = doc["device"];
  deviceConfig.name = device["name"].as<String>();
  deviceConfig.role = device["role"].as<String>();
  deviceConfig.tcpPort = device["tcpPort"];
  deviceConfig.syncPort = device["syncPort"];
  
  return true;
}

bool ConfigManager::writeConfigFile() {
  // 创建JSON文档
  DynamicJsonDocument doc(4096);
  
  // 添加网络配置
  JsonObject network = doc.createNestedObject("network");
  network["ssid"] = networkConfig.ssid;
  network["password"] = networkConfig.password;
  network["dhcpEnabled"] = networkConfig.dhcpEnabled;
  network["ip"] = networkConfig.ip;
  network["gateway"] = networkConfig.gateway;
  network["subnet"] = networkConfig.subnet;
  
  // 添加RS485配置
  JsonObject rs485 = doc.createNestedObject("rs485");
  rs485["baudRate"] = rs485Config.baudRate;
  rs485["dataBits"] = rs485Config.dataBits;
  rs485["parity"] = rs485Config.parity;
  rs485["stopBits"] = rs485Config.stopBits;
  
  // 添加设备配置
  JsonObject device = doc.createNestedObject("device");
  device["name"] = deviceConfig.name;
  device["role"] = deviceConfig.role;
  device["tcpPort"] = deviceConfig.tcpPort;
  device["syncPort"] = deviceConfig.syncPort;
  
  // 打开配置文件进行写入
  File configFile = SPIFFS.open(CONFIG_FILE_PATH, "w");
  if (!configFile) {
    Serial.println("Failed to open config file for writing");
    return false;
  }
  
  // 序列化JSON到文件
  if (serializeJson(doc, configFile) == 0) {
    Serial.println("Failed to write config file");
    configFile.close();
    return false;
  }
  
  configFile.close();
  return true;
}