#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// 设备角色定义
#define DEVICE_ROLE_MASTER_STR "master"
#define DEVICE_ROLE_SLAVE_STR  "slave"

// 网络配置默认值
// #define DEFAULT_SSID "Sina Plaza Office"
// #define DEFAULT_PASSWORD "urtheone"
// #define DEFAULT_SSID "胡子的理想L7"
// #define DEFAULT_PASSWORD "qudfimakmge9242"
// #define DEFAULT_SSID "David的iPhone"
// #define DEFAULT_PASSWORD "11111111"
#define DEFAULT_SSID "david-bj"
#define DEFAULT_PASSWORD "xiaopang800323"

// RS485配置默认值
#define DEFAULT_BAUD_RATE 9600
#define DEFAULT_DATA_BITS 8
#define DEFAULT_PARITY 0  // 0: None, 1: Odd, 2: Even
#define DEFAULT_STOP_BITS 1

// 设备配置默认值
#define DEFAULT_MASTER_NAME "WiFly485_Master"
#define DEFAULT_SLAVE_NAME "WiFly485_Slave"
#define DEFAULT_MASTER_TCP_PORT 8888

// 系统配置
// TCP缓冲区配置
#define TCP_RECEIVE_BUFFER_SIZE 1024  // 增加接收缓冲区大小
#define TCP_MAX_PACKET_SIZE 1024      // 增加最大数据包大小
#define TCP_RECEIVE_TIMEOUT 2000      // 接收超时时间(毫秒)

// LED配置
#define LED_PIN 2  // GPIO2作为LED引脚

#endif // CONFIG_H