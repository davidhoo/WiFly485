#include <Arduino.h>
#include "device.h"
#include "logger.h"
#include "wifi_manager.h"
#include "mdns_service.h"
#include "rs485.h"
#include "led_indicator.h"
#include "error_handler.h"
#include "tcp_client.h"
#include "tcp_server.h"

// 全局变量
Device device;
WiFiManager wifiManager;
MDNSService mdnsService;
RS485 rs485;
TCPClient tcpClient;
TCPServer tcpServer;
LEDPriority ledPriority = LEDPriority::PRIORITY_LOW;
LEDPriority previousPriority = LEDPriority::PRIORITY_LOW;
LEDIndicator ledIndicator(LED_PIN); // 使用GPIO2作为LED引脚

// 错误处理器
extern ErrorHandler errorHandler;

// 回调函数声明
void onWiFiConnectionStatusChanged(WiFiConnectionStatus status);
void onTCPConnectionStatusChanged(TCPConnectionStatus status);
void onTCPDataReceived(const uint8_t* data, size_t length);
void onTCPServerStatusChanged(TCPServerStatus status);
void onTCPClientConnectionChanged(int clientIndex, bool connected);
void onTCPServerDataReceived(int clientIndex, const uint8_t* data, size_t length);
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
  
  // 初始化TCP客户端（仅从设备需要）
  if (device.isSlave()) {
    if (!tcpClient.begin(&device, &mdnsService)) {
      LOG_E("Main", "TCP客户端初始化失败");
      return;
    }
    
    // 设置TCP连接状态回调函数
    tcpClient.setConnectionStatusCallback(onTCPConnectionStatusChanged);
    tcpClient.setDataReceivedCallback(onTCPDataReceived);
    
    LOG_I("Main", "TCP客户端初始化成功");
  }
  
  // 初始化TCP服务端（仅主设备需要）
  if (device.isMaster()) {
    if (!tcpServer.begin(&device, DEFAULT_MASTER_TCP_PORT)) {
      LOG_E("Main", "TCP服务端初始化失败");
      return;
    }
    
    // 设置TCP服务端回调函数
    tcpServer.setServerStatusCallback(onTCPServerStatusChanged);
    tcpServer.setClientConnectionCallback(onTCPClientConnectionChanged);
    tcpServer.setDataReceivedCallback(onTCPServerDataReceived);
    
    LOG_I("Main", "TCP服务端初始化成功");
  }
  
  // 初始化RS485通信
  if (!rs485.begin(DEFAULT_BAUD_RATE)) {
    LOG_E("Main", "RS485初始化失败");
    return;
  }
  
  LOG_I("Main", "RS485初始化成功，波特率: %d", DEFAULT_BAUD_RATE);
  
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
    LOG_D("Main", "System status - WiFi: %s",
          wifiManager.getConnectionStatusString().c_str());
    lastLogTime = currentTime;
  }
  // 处理WiFi连接
  wifiManager.handle();
  
  // 处理mDNS服务
  mdnsService.handle();
  
  // 处理TCP客户端连接（仅从设备）
  if (device.isSlave()) {
    tcpClient.handle();
    
    // 检查RS485是否有数据需要转发到TCP
    if (rs485.available() && tcpClient.getConnectionStatus() == TCP_CONNECTED) {
      uint8_t buffer[256];
      int bytesRead = rs485.receive(buffer, sizeof(buffer));
      if (bytesRead > 0) {
        LOG_D("Main", "Forwarding %d bytes from RS485 to TCP", bytesRead);
        if (!tcpClient.sendData(buffer, bytesRead)) {
          LOG_E("Main", "Failed to forward RS485 data to TCP");
        }
      }
    }
  }
  
  // 处理TCP服务端连接（仅主设备）
  if (device.isMaster()) {
    tcpServer.handle();
    
    // 检查RS485是否有数据需要广播到所有TCP客户端
    if (rs485.available() && tcpServer.getConnectedClientCount() > 0) {
      uint8_t buffer[256];
      int bytesRead = rs485.receive(buffer, sizeof(buffer));
      if (bytesRead > 0) {
        LOG_D("Main", "Broadcasting %d bytes from RS485 to TCP clients", bytesRead);
        if (!tcpServer.broadcastData(buffer, bytesRead)) {
          LOG_E("Main", "Failed to broadcast RS485 data to TCP clients");
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
      
      // 从设备在mDNS服务启动后开始TCP连接
      if (device.isSlave()) {
        LOG_I("Main", "Starting TCP client connection...");
        if (tcpClient.startConnection()) {
          LOG_I("Main", "TCP client connection started");
        } else {
          LOG_E("Main", "Failed to start TCP client connection");
        }
      }
      
      // 主设备在mDNS服务启动后开始TCP服务端
      if (device.isMaster()) {
        LOG_I("Main", "Starting TCP server...");
        if (tcpServer.start()) {
          LOG_I("Main", "TCP server started successfully");
        } else {
          LOG_E("Main", "Failed to start TCP server");
        }
      }
    } else {
      LOG_E("Main", "Failed to start mDNS service");
    }
  } else if (status == WIFI_DISCONNECTED) {
    // WiFi断开时停止TCP连接
    if (device.isSlave()) {
      tcpClient.stopConnection();
      LOG_I("Main", "TCP client connection stopped due to WiFi disconnection");
    }
    
    // WiFi断开时停止TCP服务端
    if (device.isMaster()) {
      tcpServer.stop();
      LOG_I("Main", "TCP server stopped due to WiFi disconnection");
    }
  }
}

// TCP连接状态回调函数实现
void onTCPConnectionStatusChanged(TCPConnectionStatus status) {
  LOG_I("Main", "TCP connection status changed to: %s",
        tcpClient.getConnectionStatusString().c_str());
  
  switch (status) {
    case TCP_CONNECTED:
      LOG_I("Main", "Successfully connected to master at %s:%d",
            tcpClient.getMasterIP().c_str(), tcpClient.getMasterPort());
      break;
    case TCP_DISCONNECTED:
      LOG_W("Main", "TCP connection disconnected");
      break;
    case TCP_ERROR:
      LOG_E("Main", "TCP connection error occurred");
      break;
    default:
      break;
  }
}

// TCP数据接收回调函数实现
void onTCPDataReceived(const uint8_t* data, size_t length) {
  LOG_I("Main", "Received %d bytes from master via TCP", length);
  
  // 将接收到的TCP数据转发到RS485
  if (rs485.send(data, length)) {
    LOG_D("Main", "Data forwarded to RS485 successfully");
  } else {
    LOG_E("Main", "Failed to forward data to RS485");
  }
}

// TCP服务端状态回调函数实现
void onTCPServerStatusChanged(TCPServerStatus status) {
  LOG_I("Main", "TCP server status changed to: %s",
        tcpServer.getServerStatusString().c_str());
}

// TCP客户端连接回调函数实现
void onTCPClientConnectionChanged(int clientIndex, bool connected) {
  if (connected) {
    ClientConnection* client = tcpServer.getClientConnection(clientIndex);
    if (client) {
      LOG_I("Main", "TCP client %d connected: %s:%d",
            clientIndex, client->clientIP.c_str(), client->clientPort);
    }
  } else {
    LOG_I("Main", "TCP client %d disconnected", clientIndex);
  }
}

// TCP服务端数据接收回调函数实现
void onTCPServerDataReceived(int clientIndex, const uint8_t* data, size_t length) {
  LOG_I("Main", "Received %d bytes from TCP client %d", length, clientIndex);
  
  // 将接收到的TCP数据转发到RS485
  if (rs485.send(data, length)) {
    LOG_D("Main", "Data from client %d forwarded to RS485 successfully", clientIndex);
  } else {
    LOG_E("Main", "Failed to forward data from client %d to RS485", clientIndex);
  }
}
