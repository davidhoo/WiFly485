#ifndef CONFIG_SYNC_H
#define CONFIG_SYNC_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include "device.h"
#include "config_manager.h"
#include "mdns_service.h"

// 配置同步状态枚举
enum ConfigSyncStatus {
  CONFIG_SYNC_DISCONNECTED = 0,
  CONFIG_SYNC_CONNECTING = 1,
  CONFIG_SYNC_CONNECTED = 2,
  CONFIG_SYNC_SYNCING = 3,
  CONFIG_SYNC_SYNCED = 4,
  CONFIG_SYNC_FAILED = 5
};

class ConfigSync {
public:
  ConfigSync();
  ~ConfigSync();
// 初始化配置同步
bool begin(Device* device, ConfigManager* configManager, MDNSService* mdnsService);  // 修改函数签名，添加mDNS服务参数


  // 处理配置同步
  void handle();

  // 获取同步状态
  ConfigSyncStatus getSyncStatus();

  // 获取同步状态字符串
  String getSyncStatusString();

  // 设置同步状态回调函数
  typedef void (*SyncStatusCallback)(ConfigSyncStatus status);
  void setSyncStatusCallback(SyncStatusCallback callback);

  // 请求同步配置（从设备调用）
  bool requestSync();

  // 强制同步配置（主设备调用）
  bool forceSync();
private:
  Device* device;
  ConfigManager* configManager;
  MDNSService* mdnsService;  // 添加mDNS服务指针
  
  WiFiServer* server;
  WiFiClient client;
  
  ConfigSyncStatus syncStatus;
  unsigned long lastSyncAttempt;
  unsigned long syncStartTime;
  
  SyncStatusCallback statusCallback;
  
  // 同步相关常量
  static const unsigned long SYNC_TIMEOUT = 15000; // 15秒同步超时
  static const unsigned long SYNC_INTERVAL = 60000; // 60秒同步间隔
  static const unsigned long RECONNECT_INTERVAL = 30000; // 30秒重连间隔
  static const size_t MAX_JSON_SIZE = 4096;
  
  // 内部辅助函数
  void updateSyncStatus(ConfigSyncStatus status);
  bool isSyncTimedOut();
  bool sendConfig();
  bool receiveConfig();
  void handleServer();
  void handleClient();
  bool connectToMaster();
  bool discoverMasterIP(IPAddress& masterIP, uint16_t& masterPort);  // 添加发现主设备IP的函数声明
  String getConfigAsJson();
  bool updateConfigFromJson(const String& json);
};


#endif // CONFIG_SYNC_H