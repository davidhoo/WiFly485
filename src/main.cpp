#include <Arduino.h>
#include "device.h"
#include "logger.h"
#include "config_manager.h"
#include "wifi_manager.h"
#include "mdns_service.h"
#include "rs485.h"
#include "tcp_protocol.h"
#include "config_sync.h"
#include "web_server.h"

// 全局变量
Device device;
Logger logger;
ConfigManager configManager;
WiFiManager wifiManager;
MDNSService mdnsService;
RS485 rs485;
TCPProtocol tcpProtocol;
ConfigSync configSync;
WebServer webServer(configManager, device);

void setup()
{
  // 初始化串口
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("=== WiFly485 主程序 ===");
  
  // 初始化日志系统
  logger.begin();
  logger.setLogLevel(LOG_LEVEL_VERBOSE);
  
  LOG_I("Main", "开始主程序初始化");
  
  // 初始化设备
  if (!device.begin()) {
    LOG_E("Main", "设备初始化失败");
    return;
  }
  
  LOG_I("Main", "设备初始化成功，角色: %s", device.getRoleString().c_str());
  
  // 初始化配置管理器
  if (!configManager.begin()) {
    LOG_E("Main", "配置管理器初始化失败");
    return;
  }
  
  LOG_I("Main", "配置管理器初始化成功");
  
  // 初始化WiFi管理器
  if (!wifiManager.begin(&configManager, &device)) {
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
  RS485Config rs485Config = configManager.getRS485Config();
  if (!rs485.begin(rs485Config.baudRate)) {
    LOG_E("Main", "RS485初始化失败");
    return;
  }
  
  LOG_I("Main", "RS485初始化成功，波特率: %d", rs485Config.baudRate);
  
  // 初始化TCP协议
  if (!tcpProtocol.begin(&device, &rs485)) {
    LOG_E("Main", "TCP协议初始化失败");
    return;
  }
  
  LOG_I("Main", "TCP协议初始化成功");
  
  // 初始化配置同步
  if (!configSync.begin(&device, &configManager)) {
    LOG_E("Main", "配置同步初始化失败");
    return;
  }
  
  LOG_I("Main", "配置同步初始化成功");
  
  // 初始化Web服务器
  webServer.begin();
  LOG_I("Main", "Web服务器初始化成功");
  
  LOG_I("Main", "主程序初始化完成");
  Serial.println("=== 主程序初始化完成 ===");
}

void loop()
{
  // 处理WiFi连接
  wifiManager.handle();
  
  // 处理mDNS服务
  mdnsService.handle();
  
  // 处理TCP协议
  tcpProtocol.handle();
  
  // 处理配置同步
  configSync.handle();
  
  // 处理Web服务器
  webServer.handleClient();
  
  // 短暂延迟以避免过度占用CPU
  delay(10);
}
