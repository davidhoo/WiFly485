#ifndef DEVICE_H
#define DEVICE_H

#include <Arduino.h>
#include "config.h"

// 设备角色枚举
enum DeviceRole {
  DEVICE_ROLE_UNKNOWN = 0,
  DEVICE_ROLE_MASTER_ENUM = 1,
  DEVICE_ROLE_SLAVE_ENUM = 2
};

class Device {
public:
  Device();
  ~Device();

  // 初始化设备
  bool begin();

  // 获取设备角色
  DeviceRole getRole();
  
  // 设置设备角色
  void setRole(DeviceRole role);
  
  // 获取设备角色字符串
  String getRoleString();
  
  // 判断是否为主设备
  bool isMaster();
  
  // 判断是否为从设备
  bool isSlave();
  
  // 获取设备名称
  String getName();
  
  // 设置设备名称
  void setName(const String& name);

private:
  DeviceRole role;
  String name;
  
  // 初始化设备角色
  void initializeRole();
  
  // 初始化设备名称
  void initializeName();
};

#endif // DEVICE_H