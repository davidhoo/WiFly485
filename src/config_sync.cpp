#include "config_sync.h"
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>

ConfigSync::ConfigSync() :
  device(nullptr),
  configManager(nullptr),
  server(nullptr),
  syncStatus(CONFIG_SYNC_DISCONNECTED),
  lastSyncAttempt(0),
  syncStartTime(0),
  statusCallback(nullptr) {
}

ConfigSync::~ConfigSync() {
  if (server) {
    delete server;
  }
  
  if (client.connected()) {
    client.stop();
  }
}

bool ConfigSync::begin(Device* device, ConfigManager* configManager) {
  this->device = device;
  this->configManager = configManager;
  
  if (!this->device || !this->configManager) {
    Serial.println("ConfigSync: Invalid device or configManager");
    return false;
  }
  
  // 根据设备角色初始化服务器或客户端
  if (this->device->isMaster()) {
    // 主设备创建服务器，监听同步端口
    DeviceConfig deviceConfig = configManager->getDeviceConfig();
    server = new WiFiServer(deviceConfig.syncPort);
    server->begin();
    Serial.printf("ConfigSync: Server started on port %d\n", deviceConfig.syncPort);
  } else {
    // 从设备不需要创建服务器
    server = nullptr;
    Serial.println("ConfigSync: Client mode initialized");
  }
  
  return true;
}

void ConfigSync::handle() {
  // 处理配置同步
  if (device->isMaster()) {
    // 主设备处理服务器连接
    handleServer();
  } else {
    // 从设备处理客户端连接
    handleClient();
  }
  
  // 处理同步状态
  if (syncStatus == CONFIG_SYNC_CONNECTING || syncStatus == CONFIG_SYNC_SYNCING) {
    // 检查同步是否超时
    if (isSyncTimedOut()) {
      Serial.println("ConfigSync: Sync timeout");
      updateSyncStatus(CONFIG_SYNC_FAILED);
      if (client.connected()) {
        client.stop();
      }
    }
  } else if (syncStatus == CONFIG_SYNC_FAILED) {
    // 尝试重连（仅从设备）
    if (!device->isMaster()) {
      unsigned long currentTime = millis();
      if (currentTime - lastSyncAttempt > RECONNECT_INTERVAL) {
        lastSyncAttempt = currentTime;
        connectToMaster();
      }
    }
  }
}

ConfigSyncStatus ConfigSync::getSyncStatus() {
  return syncStatus;
}

String ConfigSync::getSyncStatusString() {
  switch (syncStatus) {
    case CONFIG_SYNC_DISCONNECTED:
      return "Disconnected";
    case CONFIG_SYNC_CONNECTING:
      return "Connecting";
    case CONFIG_SYNC_CONNECTED:
      return "Connected";
    case CONFIG_SYNC_SYNCING:
      return "Syncing";
    case CONFIG_SYNC_SYNCED:
      return "Synced";
    case CONFIG_SYNC_FAILED:
      return "Failed";
    default:
      return "Unknown";
  }
}

void ConfigSync::setSyncStatusCallback(SyncStatusCallback callback) {
  statusCallback = callback;
}

bool ConfigSync::requestSync() {
  // 从设备请求同步配置
  if (device->isMaster()) {
    Serial.println("ConfigSync: Master device cannot request sync");
    return false;
  }
  
  // 如果还没有连接到主设备，先连接
  if (!client.connected()) {
    if (!connectToMaster()) {
      return false;
    }
  }
  
  // 发送同步请求
  String request = "SYNC_CONFIG";
  size_t sent = client.write(request.c_str(), request.length());
  if (sent != request.length()) {
    Serial.println("ConfigSync: Failed to send sync request");
    return false;
  }
  
  client.flush();
  updateSyncStatus(CONFIG_SYNC_SYNCING);
  syncStartTime = millis();
  
  return true;
}

bool ConfigSync::forceSync() {
  // 主设备强制同步配置
  if (!device->isMaster()) {
    Serial.println("ConfigSync: Slave device cannot force sync");
    return false;
  }
  
  // 如果有从设备连接，发送配置
  if (client.connected()) {
    updateSyncStatus(CONFIG_SYNC_SYNCING);
    syncStartTime = millis();
    return sendConfig();
  }
  
  return false;
}

void ConfigSync::updateSyncStatus(ConfigSyncStatus status) {
  if (syncStatus != status) {
    syncStatus = status;
    
    // 调用回调函数
    if (statusCallback) {
      statusCallback(status);
    }
    
    // 打印状态变化
    Serial.printf("ConfigSync: Sync status changed to %s\n", getSyncStatusString().c_str());
  }
}

bool ConfigSync::isSyncTimedOut() {
  return (millis() - syncStartTime) > SYNC_TIMEOUT;
}

bool ConfigSync::sendConfig() {
  // 检查连接状态
  if (!client.connected()) {
    Serial.println("ConfigSync: Client not connected");
    return false;
  }
  
  // 获取配置JSON字符串
  String configJson = getConfigAsJson();
  if (configJson.length() == 0) {
    Serial.println("ConfigSync: Failed to get config as JSON");
    return false;
  }
  
  // 发送配置JSON长度
  uint32_t jsonLength = configJson.length();
  size_t lengthSent = client.write((uint8_t*)&jsonLength, sizeof(jsonLength));
  if (lengthSent != sizeof(jsonLength)) {
    Serial.println("ConfigSync: Failed to send JSON length");
    return false;
  }
  
  // 发送配置JSON
  size_t jsonSent = client.write(configJson.c_str(), jsonLength);
  if (jsonSent != jsonLength) {
    Serial.println("ConfigSync: Failed to send JSON config");
    return false;
  }
  
  // 确保数据发送完成
  client.flush();
  
  Serial.println("ConfigSync: Config sent successfully");
  return true;
}

bool ConfigSync::receiveConfig() {
  // 检查连接状态
  if (!client.connected()) {
    return false;
  }
  
  // 检查是否有足够的数据可读（至少包含长度字段）
  if (client.available() < sizeof(uint32_t)) {
    return false;
  }
  
  // 读取JSON长度
  uint32_t jsonLength;
  size_t lengthRead = client.readBytes((uint8_t*)&jsonLength, sizeof(jsonLength));
  if (lengthRead != sizeof(jsonLength)) {
    Serial.println("ConfigSync: Failed to read JSON length");
    return false;
  }
  
  // 检查长度是否有效
  if (jsonLength == 0 || jsonLength > MAX_JSON_SIZE) {
    Serial.println("ConfigSync: Invalid JSON length");
    return false;
  }
  
  // 分配缓冲区并读取JSON数据
  std::unique_ptr<char[]> jsonBuffer(new char[jsonLength + 1]);
  if (client.available() < jsonLength) {
    // 数据不完整
    Serial.println("ConfigSync: Incomplete JSON data");
    return false;
  }
  
  size_t jsonRead = client.readBytes(jsonBuffer.get(), jsonLength);
  if (jsonRead != jsonLength) {
    Serial.println("ConfigSync: Failed to read JSON data");
    return false;
  }
  
  // 确保字符串以null结尾
  jsonBuffer[jsonLength] = '\0';
  
  // 更新配置
  String jsonStr(jsonBuffer.get());
  if (!updateConfigFromJson(jsonStr)) {
    Serial.println("ConfigSync: Failed to update config from JSON");
    return false;
  }
  
  Serial.println("ConfigSync: Config received and updated successfully");
  return true;
}

void ConfigSync::handleServer() {
  if (!server) {
    return;
  }
  
  // 检查是否有新的客户端连接
  WiFiClient newClient = server->available();
  if (newClient) {
    if (!client.connected()) {
      // 接受新连接
      client = newClient;
      updateSyncStatus(CONFIG_SYNC_CONNECTED);
      Serial.printf("ConfigSync: New client connected from %s\n", client.remoteIP().toString().c_str());
    } else {
      // 已经有连接，拒绝新连接
      newClient.stop();
      Serial.println("ConfigSync: New client rejected, already connected");
    }
  }
  
  // 处理现有连接
  if (client.connected()) {
    // 检查是否有数据可读
    if (client.available()) {
      // 读取请求
      char request[32];
      int bytesRead = client.readBytesUntil('\n', request, sizeof(request) - 1);
      if (bytesRead > 0) {
        request[bytesRead] = '\0';
        String requestStr(request);
        requestStr.trim();
        
        if (requestStr == "SYNC_CONFIG") {
          // 发送配置
          updateSyncStatus(CONFIG_SYNC_SYNCING);
          syncStartTime = millis();
          
          if (sendConfig()) {
            updateSyncStatus(CONFIG_SYNC_SYNCED);
          } else {
            updateSyncStatus(CONFIG_SYNC_FAILED);
          }
        } else {
          Serial.printf("ConfigSync: Unknown request: %s\n", requestStr.c_str());
        }
      }
    }
  } else {
    // 没有连接的客户端
    updateSyncStatus(CONFIG_SYNC_DISCONNECTED);
    
    // 定期自动同步（主设备主动推送配置）
    unsigned long currentTime = millis();
    if (currentTime - lastSyncAttempt > SYNC_INTERVAL) {
      lastSyncAttempt = currentTime;
      // 这里可以实现主动推送配置的逻辑
    }
  }
}

void ConfigSync::handleClient() {
  // 检查是否需要连接到主设备
  if (!client.connected()) {
    updateSyncStatus(CONFIG_SYNC_DISCONNECTED);
    unsigned long currentTime = millis();
    if (currentTime - lastSyncAttempt > RECONNECT_INTERVAL) {
      lastSyncAttempt = currentTime;
      connectToMaster();
    }
  } else {
    // 处理数据传输
    updateSyncStatus(CONFIG_SYNC_CONNECTED);
    
    // 检查是否有数据可读（接收配置）
    if (client.available()) {
      updateSyncStatus(CONFIG_SYNC_SYNCING);
      syncStartTime = millis();
      
      if (receiveConfig()) {
        updateSyncStatus(CONFIG_SYNC_SYNCED);
        // 保存配置到文件
        configManager->saveConfig();
      } else {
        updateSyncStatus(CONFIG_SYNC_FAILED);
      }
    }
    
    // 定期请求同步配置
    unsigned long currentTime = millis();
    if (currentTime - lastSyncAttempt > SYNC_INTERVAL) {
      lastSyncAttempt = currentTime;
      requestSync();
    }
  }
}

bool ConfigSync::connectToMaster() {
  // 这里需要实现连接到主设备的逻辑
  // 在实际应用中，可能需要通过mDNS或其他方式发现主设备的IP地址
  // 为了简化，我们假设主设备的IP地址是已知的或通过配置获取的
  
  // 示例代码，实际应用中需要替换为实际的主设备IP地址获取方式
  IPAddress masterIP(192, 168, 1, 100); // 示例IP地址
  
  DeviceConfig deviceConfig = configManager->getDeviceConfig();
  Serial.printf("ConfigSync: Connecting to master at %s:%d\n", masterIP.toString().c_str(), deviceConfig.syncPort);
  
  updateSyncStatus(CONFIG_SYNC_CONNECTING);
  syncStartTime = millis();
  
  // 尝试连接到主设备
  if (client.connect(masterIP, deviceConfig.syncPort)) {
    Serial.println("ConfigSync: Connected to master");
    updateSyncStatus(CONFIG_SYNC_CONNECTED);
    return true;
  } else {
    Serial.println("ConfigSync: Failed to connect to master");
    updateSyncStatus(CONFIG_SYNC_FAILED);
    return false;
  }
}

String ConfigSync::getConfigAsJson() {
  // 创建JSON文档
  DynamicJsonDocument doc(MAX_JSON_SIZE);
  
  // 获取当前配置
  NetworkConfig networkConfig = configManager->getNetworkConfig();
  RS485Config rs485Config = configManager->getRS485Config();
  DeviceConfig deviceConfig = configManager->getDeviceConfig();
  
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
  
  // 序列化JSON到字符串
  String jsonStr;
  serializeJson(doc, jsonStr);
  
  return jsonStr;
}

bool ConfigSync::updateConfigFromJson(const String& json) {
  // 解析JSON
  DynamicJsonDocument doc(MAX_JSON_SIZE);
  DeserializationError error = deserializeJson(doc, json);
  if (error) {
    Serial.println("ConfigSync: Failed to parse config JSON");
    return false;
  }
  
  // 解析网络配置
  if (doc.containsKey("network")) {
    JsonObject network = doc["network"];
    NetworkConfig networkConfig;
    networkConfig.ssid = network["ssid"].as<String>();
    networkConfig.password = network["password"].as<String>();
    networkConfig.dhcpEnabled = network["dhcpEnabled"];
    networkConfig.ip = network["ip"].as<String>();
    networkConfig.gateway = network["gateway"].as<String>();
    networkConfig.subnet = network["subnet"].as<String>();
    configManager->setNetworkConfig(networkConfig);
  }
  
  // 解析RS485配置
  if (doc.containsKey("rs485")) {
    JsonObject rs485 = doc["rs485"];
    RS485Config rs485Config;
    rs485Config.baudRate = rs485["baudRate"];
    rs485Config.dataBits = rs485["dataBits"];
    rs485Config.parity = rs485["parity"];
    rs485Config.stopBits = rs485["stopBits"];
    configManager->setRS485Config(rs485Config);
  }
  
  // 解析设备配置（注意：设备角色不应被从设备更改）
  if (doc.containsKey("device")) {
    JsonObject device = doc["device"];
    DeviceConfig deviceConfig = configManager->getDeviceConfig(); // 获取当前配置
    deviceConfig.name = device["name"].as<String>();
    // 不更新角色，因为从设备不应更改自己的角色
    deviceConfig.tcpPort = device["tcpPort"];
    deviceConfig.syncPort = device["syncPort"];
    configManager->setDeviceConfig(deviceConfig);
  }
  
  // 验证配置
  if (!configManager->validateConfig()) {
    Serial.println("ConfigSync: Invalid config received");
    return false;
  }
  
  return true;
}