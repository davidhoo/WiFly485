#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// 设备角色定义
#define DEVICE_ROLE_MASTER_STR "master"
#define DEVICE_ROLE_SLAVE_STR  "slave"

// 网络配置默认值
#define DEFAULT_SSID "WiFly485_Network"
#define DEFAULT_PASSWORD "default_password"
#define DEFAULT_DHCP_ENABLED true
#define DEFAULT_IP "0.0.0.0"
#define DEFAULT_GATEWAY "0.0.0.0"
#define DEFAULT_SUBNET "255.255.255.0"

// RS485配置默认值
#define DEFAULT_BAUD_RATE 9600
#define DEFAULT_DATA_BITS 8
#define DEFAULT_PARITY 0  // 0: None, 1: Odd, 2: Even
#define DEFAULT_STOP_BITS 1

// 设备配置默认值
#define DEFAULT_MASTER_NAME "WiFly485_Master"
#define DEFAULT_SLAVE_NAME "WiFly485_Slave"
#define DEFAULT_MASTER_TCP_PORT 8888
#define DEFAULT_SYNC_PORT 8889

// 系统配置
#define DEFAULT_CONFIG_FILE_PATH "/config.json"
#define SPIFFS_MAX_SIZE 4096

#endif // CONFIG_H