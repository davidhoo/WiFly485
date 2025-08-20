#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include "device.h"

// WiFi连接状态枚举
enum WiFiConnectionStatus {
  WIFI_DISCONNECTED = 0,
  WIFI_CONNECTING = 1,
  WIFI_CONNECTED = 2,
  WIFI_CONNECTION_FAILED = 3
};

class WiFiManager {
public:
  WiFiManager();
  ~WiFiManager();

  // 初始化WiFi管理器
  bool begin(Device* device);

  // 连接到WiFi网络
  bool connect();

  // 发送HTTP请求到http://1.1.1.1并输出返回的内容
  void sendHTTPRequest();

  // 断开WiFi连接
  void disconnect();

  // 检查WiFi连接状态
  WiFiConnectionStatus getConnectionStatus();

  // 获取WiFi状态字符串
  String getConnectionStatusString();

  // 获取本地IP地址
  IPAddress getLocalIP();

  // 获取WiFi信号强度
  int32_t getRSSI();

  // 处理WiFi事件
  // 注意：此函数不再自动触发第一次连接，第一次连接需要外部显式调用 connect() 函数
  // 此函数只处理连接超时和重连逻辑
  void handle();

  // 设置连接状态回调函数
  typedef void (*ConnectionStatusCallback)(WiFiConnectionStatus status);
  void setConnectionStatusCallback(ConnectionStatusCallback callback);

private:
  Device* device;
  
  WiFiConnectionStatus connectionStatus;
  unsigned long lastConnectionAttempt;
  unsigned long connectionStartTime;
  
  // 重试相关变量
  unsigned int retryCount;
  unsigned long currentReconnectInterval;
  
  ConnectionStatusCallback statusCallback;
  
  // 静态事件处理函数
  static void onWiFiEventStatic(WiFiEvent_t event);
  
  // 内部辅助函数
  void setupStationMode();
  void onWiFiEvent(WiFiEvent_t event);
  void updateConnectionStatus(WiFiConnectionStatus status);
  bool isConnectionTimedOut();
  
  // 重连相关
  static const unsigned long RECONNECT_INTERVAL = 30000; // 30秒重连间隔
  static const unsigned long CONNECTION_TIMEOUT = 15000; // 15秒连接超时
};

#endif // WIFI_MANAGER_H