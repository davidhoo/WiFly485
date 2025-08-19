#include "wifi_manager.h"
#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h>
#include "error_handler.h"

// 静态成员变量，用于在静态事件处理函数中访问实例
static WiFiManager* g_wifiManagerInstance = nullptr;

WiFiManager::WiFiManager() :
  configManager(nullptr),
  device(nullptr),
  connectionStatus(WIFI_DISCONNECTED),
  lastConnectionAttempt(0),
  connectionStartTime(0),
  apModeEnabled(false),
  masterStartTime(0),
  masterTimeoutChecked(false),
  slaveStartTime(0),
  slaveTimeoutChecked(false),
  statusCallback(nullptr) {
  // 构造函数
  // 设置全局实例指针
  g_wifiManagerInstance = this;
}

WiFiManager::~WiFiManager() {
  // 析构函数
  disconnect();
}

bool WiFiManager::begin(ConfigManager* configManager, Device* device) {
  this->configManager = configManager;
  this->device = device;
  if (!this->configManager || !this->device) {
    REPORT_ERROR(ERROR_INVALID_PARAMETER, "WiFiManager", "Invalid configManager or device");
    return false;
  }
  
  // 注册WiFi事件处理函数
  WiFi.onEvent(WiFiManager::onWiFiEventStatic);
  
  // 根据设备角色设置WiFi模式
  if (device->isMaster()) {
    // 主设备默认启用AP模式
    WiFi.mode(WIFI_AP_STA);
  } else {
    // 从设备也使用AP+STA模式，以便可以作为热点或连接到网络
    WiFi.mode(WIFI_AP_STA);
  }
  
  return true;
}

bool WiFiManager::connect() {
  if (!configManager || !device) {
    REPORT_ERROR(ERROR_INVALID_PARAMETER, "WiFiManager", "Not initialized");
    return false;
  }
  
  // 如果已经在连接中，直接返回
  if (connectionStatus == WIFI_CONNECTING) {
    return true;
  }
  
  // 获取网络配置
  NetworkConfig networkConfig = configManager->getNetworkConfig();
  
  // 检查SSID是否有效
  if (networkConfig.ssid.length() == 0) {
    REPORT_ERROR(ERROR_INVALID_PARAMETER, "WiFiManager", "Invalid SSID");
    updateConnectionStatus(WIFI_CONNECTION_FAILED);
    return false;
  }
  
  // 更新连接状态
  updateConnectionStatus(WIFI_CONNECTING);
  connectionStartTime = millis();
  
  // 配置静态IP（如果需要）
  if (!networkConfig.dhcpEnabled) {
    IPAddress ip, gateway, subnet;
    if (ip.fromString(networkConfig.ip) && 
        gateway.fromString(networkConfig.gateway) && 
        subnet.fromString(networkConfig.subnet)) {
      WiFi.config(ip, gateway, subnet);
    }
  }
  
  // 连接到WiFi网络
  Serial.printf("WiFiManager: Connecting to %s\n", networkConfig.ssid.c_str());
  WiFi.begin(networkConfig.ssid.c_str(), networkConfig.password.c_str());
  
  return true;
}

bool WiFiManager::connectToRouterWiFi() {
  // 更新连接状态
  updateConnectionStatus(WIFI_CONNECTING);
  connectionStartTime = millis();
  
  // 连接到指定的路由器WiFi
  const char* ssid = "David的iPhone";
  // const char* password = "qudfimakmge9242";
  const char* password = "11111111";
  
  Serial.printf("WiFiManager: Connecting to router WiFi %s\n", ssid);
  WiFi.begin(ssid, password);
  
  
  return true;
}

bool WiFiManager::startAP() {
  if (!configManager || !device) {
    REPORT_ERROR(ERROR_INVALID_PARAMETER, "WiFiManager", "Not initialized");
    return false;
  }
  
  // 获取设备配置
  DeviceConfig deviceConfig = configManager->getDeviceConfig();
  
  // 生成AP名称和密码
  String apName = deviceConfig.name + "_AP";
  String apPassword = "wifly485"; // 默认密码
  
  Serial.printf("WiFiManager: Starting AP %s\n", apName.c_str());
  
  // 启动AP模式
  bool result = WiFi.softAP(apName.c_str(), apPassword.c_str());
  
  if (result) {
    apModeEnabled = true;
    Serial.printf("WiFiManager: AP started with IP %s\n", WiFi.softAPIP().toString().c_str());
  } else {
    Serial.println("WiFiManager: Failed to start AP");
  }
  
  return result;
}

void WiFiManager::disconnect() {
  WiFi.disconnect();
  updateConnectionStatus(WIFI_DISCONNECTED);
  apModeEnabled = false;
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
      Serial.println("WiFiManager: Connection timeout");
      updateConnectionStatus(WIFI_CONNECTION_FAILED);
      WiFi.disconnect();
    }
  } else if (connectionStatus == WIFI_CONNECTION_FAILED) {
    // 尝试重连
    unsigned long currentTime = millis();
    if (currentTime - lastConnectionAttempt > RECONNECT_INTERVAL) {
      lastConnectionAttempt = currentTime;
      connect();
    }
  } else if (connectionStatus == WIFI_DISCONNECTED && device) {
    // 主设备或从设备60秒超时检测
    
    // 检查设备连接超时
    checkDeviceTimeout();
  }
  
  // 如果AP模式未启用，启动AP模式（主设备和从设备都启动AP）
  if (device && !apModeEnabled) {
    startAP();
  }
}

void WiFiManager::setConnectionStatusCallback(ConnectionStatusCallback callback) {
  statusCallback = callback;
}

void WiFiManager::setupStationMode() {
  WiFi.mode(WIFI_STA);
}

void WiFiManager::setupAPMode() {
  WiFi.mode(WIFI_AP);
}

void WiFiManager::onWiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case WIFI_EVENT_STAMODE_CONNECTED:
      Serial.println("WiFiManager: Station connected to AP");
      break;
      
    case WIFI_EVENT_STAMODE_DISCONNECTED:
      Serial.println("WiFiManager: Station disconnected from AP");
      updateConnectionStatus(WIFI_DISCONNECTED);
      break;
      
    case WIFI_EVENT_STAMODE_GOT_IP:
      Serial.printf("WiFiManager: Station got IP: %s\n", WiFi.localIP().toString().c_str());
      updateConnectionStatus(WIFI_CONNECTED);
      break;
      
    case WIFI_EVENT_STAMODE_DHCP_TIMEOUT:
      Serial.println("WiFiManager: Station DHCP timeout");
      updateConnectionStatus(WIFI_CONNECTION_FAILED);
      break;
      
    default:
      break;
  }
}

void WiFiManager::updateConnectionStatus(WiFiConnectionStatus status) {
  if (connectionStatus != status) {
    connectionStatus = status;
    
    // 调用回调函数
    if (statusCallback) {
      statusCallback(status);
    }
    
    // 打印状态变化
    Serial.printf("WiFiManager: Connection status changed to %s\n", getConnectionStatusString().c_str());
  }
}

bool WiFiManager::isConnectionTimedOut() {
  return (millis() - connectionStartTime) > CONNECTION_TIMEOUT;
}

void WiFiManager::checkDeviceTimeout() {
  // 使用引用避免重复代码
  unsigned long& startTime = device->isMaster() ? masterStartTime : slaveStartTime;
  bool& timeoutChecked = device->isMaster() ? masterTimeoutChecked : slaveTimeoutChecked;
  
  // 设置开始时间
  if (startTime == 0) {
    startTime = millis();
  }
  
  // 检查超时
  if (!timeoutChecked && (millis() - startTime) > APMODE_CONNECTION_TIMEOUT) {
    Serial.printf("WiFiManager: %s connection timeout, connecting to router WiFi\n",
                   device->isMaster() ? "Master" : "Slave");
    timeoutChecked = true;
    connectToRouterWiFi(); // 连接到指定的路由器WiFi
  }
}

// 静态WiFi事件处理函数
void WiFiManager::onWiFiEventStatic(WiFiEvent_t event) {
  // 通过全局实例指针调用成员函数
  if (g_wifiManagerInstance != nullptr) {
    g_wifiManagerInstance->onWiFiEvent(event);
  }
}