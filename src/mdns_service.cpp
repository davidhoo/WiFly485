#include "mdns_service.h"
#include <ESP8266mDNS.h>
#include "config.h"
#include "error_handler.h"

MDNSService::MDNSService() :
  device(nullptr),
  wifiManager(nullptr),
  serviceStatus(MDNS_SERVICE_STOPPED),
  servicePort(DEFAULT_MASTER_TCP_PORT),
  statusCallback(nullptr) {
  // 构造函数
}

MDNSService::~MDNSService() {
  // 析构函数
  stop();
}

bool MDNSService::begin(Device* device, WiFiManager* wifiManager) {
  this->device = device;
  this->wifiManager = wifiManager;
  if (!this->device || !this->wifiManager) {
    REPORT_ERROR(ERROR_INVALID_PARAMETER, "MDNSService", "Invalid device or wifiManager");
    return false;
  }
  
  // 根据设备角色设置服务名称和主机名
  if (device->isMaster()) {
    serviceName = "wifly485-master";
    serviceInstanceName = device->getName() + "_master";
  } else {
    serviceName = "wifly485-slave";
    serviceInstanceName = device->getName() + "_slave";
  }
  
  return true;
}

bool MDNSService::start() {
  if (!device || !wifiManager) {
    REPORT_ERROR(ERROR_INVALID_PARAMETER, "MDNSService", "Not initialized");
    return false;
  }
  
  // 检查WiFi连接状态
  if (wifiManager->getConnectionStatus() != WIFI_CONNECTED) {
    REPORT_ERROR(ERROR_WIFI_NOT_CONNECTED, "MDNSService", "WiFi not connected");
    return false;
  }
  
  // 更新服务状态
  updateServiceStatus(MDNS_SERVICE_STARTING);
  
  // 设置mDNS服务
  if (setupMDNSService()) {
    updateServiceStatus(MDNS_SERVICE_RUNNING);
    LOG_I("MDNSService", "Started %s service", serviceName.c_str());
    return true;
  } else {
    updateServiceStatus(MDNS_SERVICE_ERROR);
    REPORT_ERROR(ERROR_MDNS_FAILED, "MDNSService", "Failed to start service");
    return false;
  }
}

void MDNSService::stop() {
MDNS.close();
updateServiceStatus(MDNS_SERVICE_STOPPED);
LOG_I("MDNSService", "Stopped");
}

void MDNSService::handle() {
  // 处理mDNS查询
  MDNS.update();
  
  // 检查WiFi连接状态，如果断开则停止mDNS服务
  if (wifiManager && wifiManager->getConnectionStatus() != WIFI_CONNECTED) {
    if (serviceStatus == MDNS_SERVICE_RUNNING) {
      stop();
    }
  }
}

MDNSServiceStatus MDNSService::getStatus() {
  return serviceStatus;
}

String MDNSService::getStatusString() {
  switch (serviceStatus) {
    case MDNS_SERVICE_STOPPED:
      return "Stopped";
    case MDNS_SERVICE_STARTING:
      return "Starting";
    case MDNS_SERVICE_RUNNING:
      return "Running";
    case MDNS_SERVICE_ERROR:
      return "Error";
    default:
      return "Unknown";
  }
}

bool MDNSService::discoverMaster(String& masterIP, uint16_t& masterPort) {
// 查找主设备服务
int n = MDNS.queryService("wifly485-master", "tcp");

if (n > 0) {
  // 找到主设备
  masterIP = MDNS.IP(0).toString();
  masterPort = MDNS.port(0);
  LOG_I("MDNSService", "Found master at %s:%d", masterIP.c_str(), masterPort);
  return true;
} else {
  LOG_W("MDNSService", "No master found");
  return false;
}
}
bool MDNSService::discoverSlave(String& slaveIP, uint16_t& slavePort) {
// 查找从设备服务
int n = MDNS.queryService("wifly485-slave", "tcp");

if (n > 0) {
  // 找到从设备
  slaveIP = MDNS.IP(0).toString();
  slavePort = MDNS.port(0);
  LOG_I("MDNSService", "Found slave at %s:%d", slaveIP.c_str(), slavePort);
  return true;
} else {
  LOG_W("MDNSService", "No slave found");
  return false;
}
 }
 
 void MDNSService::setServiceStatusCallback(ServiceStatusCallback callback) {
 statusCallback = callback;
 }

void MDNSService::updateServiceStatus(MDNSServiceStatus status) {
if (serviceStatus != status) {
  serviceStatus = status;
  
  // 调用回调函数
  if (statusCallback) {
    statusCallback(status);
  }
  
  // 打印状态变化
  LOG_I("MDNSService", "Service status changed to %s", getStatusString().c_str());
}
}

bool MDNSService::setupMDNSService() {
// 初始化mDNS
// 根据需求文档，主机名应该是"wifly485-[角色]"
String hostname = "wifly485-";
if (device->isMaster()) {
  hostname += "master";
} else {
  hostname += "slave";
}

if (!MDNS.begin(hostname.c_str())) {
  REPORT_ERROR(ERROR_MDNS_FAILED, "MDNSService", "Failed to start mDNS");
  return false;
}

// 添加服务
if (device->isMaster()) {
  // 主设备添加TCP服务
  MDNS.addService("wifly485-master", "tcp", DEFAULT_MASTER_TCP_PORT);
  MDNS.addServiceTxt("wifly485-master", "tcp", "device", device->getName().c_str());
  MDNS.addServiceTxt("wifly485-master", "tcp", "role", "master");
} else {
  // 从设备添加服务
  MDNS.addService("wifly485-slave", "tcp", DEFAULT_MASTER_TCP_PORT);
  MDNS.addServiceTxt("wifly485-slave", "tcp", "device", device->getName().c_str());
  MDNS.addServiceTxt("wifly485-slave", "tcp", "role", "slave");
}
LOG_I("MDNSService", "Added service %s", serviceName.c_str());
return true;
}