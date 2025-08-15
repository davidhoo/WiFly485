#include "device.h"
#include <ESP8266WiFi.h>

Device::Device() : role(DEVICE_ROLE_UNKNOWN) {
  // 构造函数
}

Device::~Device() {
  // 析构函数
}

bool Device::begin() {
  // 初始化设备角色
  initializeRole();
  
  // 初始化设备名称
  initializeName();
  
  return true;
}

DeviceRole Device::getRole() {
  return role;
}

void Device::setRole(DeviceRole role) {
  this->role = role;
}

String Device::getRoleString() {
  switch (role) {
    case DEVICE_ROLE_MASTER_ENUM:
      return DEVICE_ROLE_MASTER_STR;
    case DEVICE_ROLE_SLAVE_ENUM:
      return DEVICE_ROLE_SLAVE_STR;
    default:
      return "unknown";
  }
}

bool Device::isMaster() {
  return role == DEVICE_ROLE_MASTER_ENUM;
}

bool Device::isSlave() {
  return role == DEVICE_ROLE_SLAVE_ENUM;
}

String Device::getName() {
  return name;
}

void Device::setName(const String& name) {
  this->name = name;
}

void Device::initializeRole() {
  // 根据编译时定义的宏来确定设备角色
#ifdef DEVICE_ROLE_MASTER
  role = DEVICE_ROLE_MASTER_ENUM;
#elif defined(DEVICE_ROLE_SLAVE)
  role = DEVICE_ROLE_SLAVE_ENUM;
#else
  role = DEVICE_ROLE_UNKNOWN;
#endif
}

void Device::initializeName() {
  // 根据设备角色设置设备名称
  if (isMaster()) {
    name = DEFAULT_MASTER_NAME;
  } else if (isSlave()) {
    name = DEFAULT_SLAVE_NAME;
  } else {
    name = "WiFly485_Unknown";
  }
}