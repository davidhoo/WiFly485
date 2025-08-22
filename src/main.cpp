#include <Arduino.h>
#include "device.h"
#include "logger.h"
#include "wifi_manager.h"
#include "mdns_service.h"
#include "rs485.h"
#include "led_indicator.h"
#include "error_handler.h"
#include "tcp_server.h"
#include "tcp_client.h"

// 全局变量
Device device;
WiFiManager wifiManager;
MDNSService mdnsService;
RS485 rs485;
TCPServer tcpServer;
TCPClient tcpClient;
LEDPriority ledPriority = LEDPriority::PRIORITY_LOW;
LEDPriority previousPriority = LEDPriority::PRIORITY_LOW;
LEDIndicator ledIndicator(LED_PIN); // 使用GPIO2作为LED引脚

// 从设备连接状态
bool masterDiscovered = false;
String masterIP = "";
uint16_t masterPort = 0;
unsigned long lastMasterDiscovery = 0;
const unsigned long MASTER_DISCOVERY_INTERVAL = 10000; // 10秒间隔重新发现主设备

// 错误处理器
extern ErrorHandler errorHandler;

// WiFi连接状态回调函数声明
void onWiFiConnectionStatusChanged(WiFiConnectionStatus status);
void setup()
{
  // 初始化串口
  Serial.begin(115200);
  delay(1000);
  
  LOG_I("Main", "=== WiFly485 主程序 ===");
  
  // 初始化日志系统
  logger.begin();
  logger.setLogLevel(LOG_LEVEL_VERBOSE);
  
  LOG_I("Main", "开始主程序初始化");
  
  // 初始化错误处理器
  errorHandler.begin();
  ErrorHandler::setGlobalErrorHandler(&errorHandler);
  LOG_I("Main", "错误处理器初始化成功");
  
  // 初始化设备
  if (!device.begin()) {
    LOG_E("Main", "设备初始化失败");
    return;
  }
  
  LOG_I("Main", "设备初始化成功，角色: %s", device.getRoleString().c_str());
  
  // 初始化WiFi管理器
  if (!wifiManager.begin(&device)) {
    LOG_E("Main", "WiFi管理器初始化失败");
    return;
  }
  
  LOG_I("Main", "WiFi管理器初始化成功");
  
  // 初始化mDNS服务
  if (!mdnsService.begin(&device, &wifiManager)) {
    LOG_E("Main", "mDNS服务初始化失败");
    return;
  }
  
  LOG_I("Main", "mDNS服务初始化成功");
  
  // 初始化RS485通信
  if (!rs485.begin(DEFAULT_BAUD_RATE)) {
    LOG_E("Main", "RS485初始化失败");
    return;
  }
  
  LOG_I("Main", "RS485初始化成功，波特率: %d", DEFAULT_BAUD_RATE);
  // 初始化TCP服务器（仅主设备）
  if (device.isMaster()) {
    if (!tcpServer.begin(&device, &rs485)) {
      LOG_E("Main", "TCP服务器初始化失败");
      return;
    }
    LOG_I("Main", "TCP服务器初始化成功，端口: %d", tcpServer.getPort());
  }
  
  // 初始化TCP客户端（仅从设备）
  if (device.isSlave()) {
    if (!tcpClient.begin(&device, &rs485)) {
      LOG_E("Main", "TCP客户端初始化失败");
      return;
    }
    LOG_I("Main", "TCP客户端初始化成功");
  }

  // 初始化LED指示器
  ledIndicator.begin();
  ledIndicator.setState(LEDState::OFF, LEDPriority::PRIORITY_LOW);
  
  // 注册WiFi连接状态回调函数
  wifiManager.setConnectionStatusCallback(onWiFiConnectionStatusChanged);
  
  // 触发第一次WiFi连接
  LOG_I("Main", "触发第一次WiFi连接");
  wifiManager.connect();
  
  LOG_I("Main", "主程序初始化完成");
  LOG_I("Main", "=== 主程序初始化完成 ===");
}
void loop()
{
  static unsigned long lastLogTime = 0;
  unsigned long currentTime = millis();
  
  // 每5秒记录一次系统状态
  // 每5秒记录一次系统状态
  if (currentTime - lastLogTime > 5000) {
    if (device.isMaster()) {
      LOG_D("Main", "System status - WiFi: %s, TCP Server: %s",
            wifiManager.getConnectionStatusString().c_str(),
            tcpServer.getStatusString().c_str());
    } else {
      LOG_D("Main", "System status - WiFi: %s, TCP Client: %s",
            wifiManager.getConnectionStatusString().c_str(),
            tcpClient.getStatusString().c_str());
    }
    lastLogTime = currentTime;
  }
  // 处理WiFi连接
  wifiManager.handle();
  
  // 处理mDNS服务
  mdnsService.handle();
  
  // 处理TCP服务器（仅主设备）
  if (device.isMaster()) {
    tcpServer.handle();
  }
  
  // 处理TCP客户端和主设备发现（仅从设备）
  if (device.isSlave()) {
    // 处理TCP客户端连接
    tcpClient.handle();
    
    // 如果WiFi已连接但TCP客户端未连接，尝试发现并连接主设备
    if (wifiManager.getConnectionStatus() == WIFI_CONNECTED &&
        !tcpClient.isConnected()) {
      
      // 定期尝试发现主设备
      if (currentTime - lastMasterDiscovery >= MASTER_DISCOVERY_INTERVAL) {
        lastMasterDiscovery = currentTime;
        
        LOG_I("Main", "Attempting to discover master device...");
        String discoveredMasterIP;
        uint16_t discoveredMasterPort;
        
        if (mdnsService.discoverMaster(discoveredMasterIP, discoveredMasterPort)) {
          // 发现主设备成功
          masterDiscovered = true;
          masterIP = discoveredMasterIP;
          masterPort = discoveredMasterPort;
          
          LOG_I("Main", "Master device discovered at %s:%d",
                masterIP.c_str(), masterPort);
          
          // 尝试连接到主设备
          if (tcpClient.connect(masterIP, masterPort)) {
            LOG_I("Main", "Successfully connected to master device");
          } else {
            LOG_W("Main", "Failed to connect to master device");
          }
        } else {
          LOG_W("Main", "No master device found");
          masterDiscovered = false;
        }
      }
    }
  }

  // 根据设备状态更新LED指示器
  // 根据设备状态更新LED指示器
  if (device.isMaster()) {
    // 主设备状态指示
    if (wifiManager.getConnectionStatus() == WIFI_CONNECTED) {
        // 检查是否有RS485数据传输，如果有则使用呼吸模式
        if (rs485.available()) {
          if (ledIndicator.getCurrentState() != LEDState::BREATHING || ledIndicator.getCurrentPriority() != LEDPriority::PRIORITY_HIGH) {
            ledIndicator.setState(LEDState::BREATHING, LEDPriority::PRIORITY_HIGH);
          }
        } else {
          if (ledIndicator.getCurrentState() != LEDState::CONNECTED || ledIndicator.getCurrentPriority() != LEDPriority::PRIORITY_NORMAL) {
            ledIndicator.setState(LEDState::CONNECTED, LEDPriority::PRIORITY_NORMAL);
          }
        }
    } else {
      // WiFi未连接
      if (wifiManager.getConnectionStatus() == WIFI_CONNECTING) {
        // 正在连接WiFi
        if (ledIndicator.getCurrentState() != LEDState::BLINK_FAST || ledIndicator.getCurrentPriority() != LEDPriority::PRIORITY_NORMAL) {
          ledIndicator.setState(LEDState::BLINK_FAST, LEDPriority::PRIORITY_NORMAL);
        }
      } else {
        // WiFi连接失败
        if (ledIndicator.getCurrentState() != LEDState::ERROR || ledIndicator.getCurrentPriority() != LEDPriority::PRIORITY_HIGH) {
          ledIndicator.setState(LEDState::ERROR, LEDPriority::PRIORITY_HIGH);
        }
      }
    }
  } else {
    // 从设备状态指示
    if (wifiManager.getConnectionStatus() == WIFI_CONNECTED) {
      if (tcpClient.isConnected()) {
        // TCP已连接到主设备
        // 检查是否有RS485数据传输，如果有则使用呼吸模式
        if (rs485.available()) {
          if (ledIndicator.getCurrentState() != LEDState::BREATHING || ledIndicator.getCurrentPriority() != LEDPriority::PRIORITY_HIGH) {
            ledIndicator.setState(LEDState::BREATHING, LEDPriority::PRIORITY_HIGH);
          }
        } else {
          if (ledIndicator.getCurrentState() != LEDState::CONNECTED || ledIndicator.getCurrentPriority() != LEDPriority::PRIORITY_NORMAL) {
            ledIndicator.setState(LEDState::CONNECTED, LEDPriority::PRIORITY_NORMAL);
          }
        }
      } else {
        // WiFi已连接但TCP未连接到主设备，使用慢闪烁表示正在寻找主设备
        if (ledIndicator.getCurrentState() != LEDState::BLINK_SLOW || ledIndicator.getCurrentPriority() != LEDPriority::PRIORITY_NORMAL) {
          ledIndicator.setState(LEDState::BLINK_SLOW, LEDPriority::PRIORITY_NORMAL);
        }
      }
    } else {
      // WiFi未连接
      if (wifiManager.getConnectionStatus() == WIFI_CONNECTING) {
        // 正在连接WiFi
        if (ledIndicator.getCurrentState() != LEDState::BLINK_FAST || ledIndicator.getCurrentPriority() != LEDPriority::PRIORITY_NORMAL) {
          ledIndicator.setState(LEDState::BLINK_FAST, LEDPriority::PRIORITY_NORMAL);
        }
      } else {
        // WiFi连接失败
        if (ledIndicator.getCurrentState() != LEDState::ERROR || ledIndicator.getCurrentPriority() != LEDPriority::PRIORITY_HIGH) {
          ledIndicator.setState(LEDState::ERROR, LEDPriority::PRIORITY_HIGH);
        }
      }
    }
  }
  
  // 更新LED指示器
  ledIndicator.update();
  
  // 短暂延迟以避免过度占用CPU
  delay(10);
}

// WiFi连接状态回调函数实现
void onWiFiConnectionStatusChanged(WiFiConnectionStatus status) {
  // 主设备或从设备在WiFi连接成功时都启动mDNS服务
  if (status == WIFI_CONNECTED) {
    LOG_I("Main", "WiFi connected, starting mDNS service...");
    if (mdnsService.start()) {
      LOG_I("Main", "mDNS service started successfully");
    } else {
      LOG_E("Main", "Failed to start mDNS service");
    }
    
    // 主设备在WiFi连接成功后启动TCP服务器
    if (device.isMaster()) {
      LOG_I("Main", "WiFi connected, starting TCP server...");
      if (tcpServer.start()) {
        LOG_I("Main", "TCP server started successfully on port %d", tcpServer.getPort());
      } else {
        LOG_E("Main", "Failed to start TCP server");
      }
    }
    
    // 从设备在WiFi连接成功后立即尝试发现并连接主设备
    if (device.isSlave()) {
      LOG_I("Main", "WiFi connected, attempting to discover and connect to master...");
      // 重置发现时间，让主循环立即尝试发现主设备
      lastMasterDiscovery = 0;
    }
  } else if (status == WIFI_DISCONNECTED) {
    // WiFi断开时，从设备断开TCP连接
    if (device.isSlave() && tcpClient.isConnected()) {
      LOG_I("Main", "WiFi disconnected, disconnecting from master...");
      tcpClient.disconnect();
      masterDiscovered = false;
    }
  }
}
