#include "wifi_manager.h"
#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h>
#include "error_handler.h"

// 静态成员变量，用于在静态事件处理函数中访问实例
static WiFiManager* g_wifiManagerInstance = nullptr;

WiFiManager::WiFiManager() :
  device(nullptr),
  connectionStatus(WIFI_DISCONNECTED),
  lastConnectionAttempt(0),
  connectionStartTime(0),
  retryCount(0),
  currentReconnectInterval(RECONNECT_INTERVAL),
  statusCallback(nullptr) {
  // 构造函数
  // 设置全局实例指针
  g_wifiManagerInstance = this;
}

WiFiManager::~WiFiManager() {
  // 析构函数
  disconnect();
}

bool WiFiManager::begin(Device* device) {
  this->device = device;
  if (!this->device) {
    REPORT_ERROR(ERROR_INVALID_PARAMETER, "WiFiManager", "Invalid device");
    return false;
  }
  
  // 注册WiFi事件处理函数
  WiFi.onEvent(WiFiManager::onWiFiEventStatic);
  
  // 设置WiFi模式为STA模式
  WiFi.mode(WIFI_STA);
  
  return true;
}

bool WiFiManager::connect() {
  if (!device) {
    REPORT_ERROR(ERROR_INVALID_PARAMETER, "WiFiManager", "Not initialized");
    return false;
  }
  
  // 如果已经在连接中，直接返回
  if (connectionStatus == WIFI_CONNECTING) {
    return true;
  }
  
  // 更新连接状态
  updateConnectionStatus(WIFI_CONNECTING);
  connectionStartTime = millis();
  
  // 更新上次连接尝试时间
  lastConnectionAttempt = millis();
  
  // 使用默认网络配置
  const char* ssid = DEFAULT_SSID;
  const char* password = DEFAULT_PASSWORD;
  
  // 检查SSID是否有效
  if (strlen(ssid) == 0) {
    REPORT_ERROR(ERROR_INVALID_PARAMETER, "WiFiManager", "Invalid SSID");
    updateConnectionStatus(WIFI_CONNECTION_FAILED);
    return false;
  }
  // 连接到WiFi网络
  LOG_I("WiFiManager", "Connecting to %s", ssid);
  WiFi.begin(ssid, password);
  
  return true;
}

void WiFiManager::disconnect() {
  updateConnectionStatus(WIFI_DISCONNECTED);
}

WiFiConnectionStatus WiFiManager::getConnectionStatus() {
  return connectionStatus;
}

String WiFiManager::getConnectionStatusString() {
  switch (connectionStatus) {
    case WIFI_DISCONNECTED:
      return "Disconnected";
    case WIFI_CONNECTING:
      return "Connecting";
    case WIFI_CONNECTED:
      return "Connected";
    case WIFI_CONNECTION_FAILED:
      return "Connection Failed";
    default:
      return "Unknown";
  }
}

IPAddress WiFiManager::getLocalIP() {
  if (connectionStatus == WIFI_CONNECTED) {
    return WiFi.localIP();
  }
  return IPAddress(0, 0, 0, 0);
}

int32_t WiFiManager::getRSSI() {
  if (connectionStatus == WIFI_CONNECTED) {
    return WiFi.RSSI();
  }
  return -100; // 无效信号强度
}

void WiFiManager::handle() {
  // 处理WiFi连接状态
  if (connectionStatus == WIFI_CONNECTING) {
    // 检查连接是否超时
    if (isConnectionTimedOut()) {
      LOG_E("WiFiManager", "Connection timeout");
      updateConnectionStatus(WIFI_CONNECTION_FAILED);
      WiFi.disconnect();
    }
  } else if (connectionStatus == WIFI_DISCONNECTED || connectionStatus == WIFI_CONNECTION_FAILED) {
    // 计算应该使用的重连间隔（基于当前重试次数+1）
    unsigned long expectedReconnectInterval = RECONNECT_INTERVAL * (retryCount + 1);
    if (expectedReconnectInterval > 300000) { // 300秒 = 5分钟
      expectedReconnectInterval = 300000;
    }
    
    // 尝试重连（处理断开连接和连接失败的情况）
    unsigned long currentTime = millis();
    if (currentTime - lastConnectionAttempt > expectedReconnectInterval) {
      lastConnectionAttempt = currentTime;
      retryCount++;
      
      // 更新重连间隔，最大不超过300秒
      currentReconnectInterval = RECONNECT_INTERVAL * retryCount;
      if (currentReconnectInterval > 300000) { // 300秒 = 5分钟
        currentReconnectInterval = 300000;
      }
      LOG_I("WiFiManager", "Reconnect attempt %d, next interval %lu ms",
                    retryCount, currentReconnectInterval);
      connect();
      connect();
    }
  }
  // 注意：第一次连接需要外部显式调用 connect() 函数，不会在 handle() 中立即触发
}

void WiFiManager::setConnectionStatusCallback(ConnectionStatusCallback callback) {
  statusCallback = callback;
}

void WiFiManager::setupStationMode() {
  WiFi.mode(WIFI_STA);
}

void WiFiManager::onWiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case WIFI_EVENT_STAMODE_CONNECTED:
      LOG_I("WiFiManager", "Station connected to AP");
      break;
      
    case WIFI_EVENT_STAMODE_DISCONNECTED:
      LOG_I("WiFiManager", "Station disconnected from AP");
      updateConnectionStatus(WIFI_DISCONNECTED);
      break;
      
    case WIFI_EVENT_STAMODE_GOT_IP:
      LOG_I("WiFiManager", "Station got IP: %s", WiFi.localIP().toString().c_str());
      updateConnectionStatus(WIFI_CONNECTED);
      break;
      
    case WIFI_EVENT_STAMODE_DHCP_TIMEOUT:
      LOG_E("WiFiManager", "Station DHCP timeout");
      updateConnectionStatus(WIFI_CONNECTION_FAILED);
      break;
      
    default:
      break;
  }
}

void WiFiManager::updateConnectionStatus(WiFiConnectionStatus status) {
  if (connectionStatus != status) {
    connectionStatus = status;
    
    // 如果连接成功，重置重试计数和重连间隔
    if (status == WIFI_CONNECTED) {
      retryCount = 0;
      currentReconnectInterval = RECONNECT_INTERVAL;
      lastConnectionAttempt = millis(); // 更新上次连接尝试时间
      LOG_I("WiFiManager", "Connection successful, reset retry count");
    }
    
    // 调用回调函数
    if (statusCallback) {
      statusCallback(status);
    }
    
    // 打印状态变化
    LOG_I("WiFiManager", "Connection status changed to %s", getConnectionStatusString().c_str());
  }
}

bool WiFiManager::isConnectionTimedOut() {
  return (millis() - connectionStartTime) > CONNECTION_TIMEOUT;
}

// 静态WiFi事件处理函数
void WiFiManager::onWiFiEventStatic(WiFiEvent_t event) {
  // 通过全局实例指针调用成员函数
  if (g_wifiManagerInstance != nullptr) {
    g_wifiManagerInstance->onWiFiEvent(event);
  }
}