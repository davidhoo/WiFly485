#ifndef MDNS_SERVICE_H
#define MDNS_SERVICE_H

#include <Arduino.h>
#include <ESP8266mDNS.h>
#include "device.h"
#include "wifi_manager.h"

// mDNS服务类型
#define MDNS_SERVICE_TYPE "_wifly485._tcp"

// mDNS服务状态枚举
enum MDNSServiceStatus {
  MDNS_SERVICE_STOPPED = 0,
  MDNS_SERVICE_STARTING = 1,
  MDNS_SERVICE_RUNNING = 2,
  MDNS_SERVICE_ERROR = 3
};

class MDNSService {
public:
  MDNSService();
  ~MDNSService();

  // 初始化mDNS服务
  bool begin(Device* device, WiFiManager* wifiManager);

  // 启动mDNS服务
  bool start();

  // 停止mDNS服务
  void stop();

  // 处理mDNS事件
  void handle();

  // 获取服务状态
  MDNSServiceStatus getStatus();

  // 获取服务状态字符串
  String getStatusString();

  // 查找主设备
  bool discoverMaster(String& masterIP, uint16_t& masterPort);

  // 查找从设备
  bool discoverSlave(String& slaveIP, uint16_t& slavePort);

  // 设置服务状态回调函数
  typedef void (*ServiceStatusCallback)(MDNSServiceStatus status);
  void setServiceStatusCallback(ServiceStatusCallback callback);

private:
  Device* device;
  WiFiManager* wifiManager;
  
  MDNSServiceStatus serviceStatus;
  String serviceName;
  String serviceInstanceName;
  uint16_t servicePort;
  
  ServiceStatusCallback statusCallback;
  
  // 内部辅助函数
  void updateServiceStatus(MDNSServiceStatus status);
  bool setupMDNSService();
  void onServiceResolveCallback();
};

#endif // MDNS_SERVICE_H