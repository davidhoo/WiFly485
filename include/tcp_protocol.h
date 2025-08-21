#ifndef TCP_PROTOCOL_H
#define TCP_PROTOCOL_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiServer.h>
#include "device.h"
#include "rs485.h"
#include "mdns_service.h"
#include "config.h"

// 前向声明
class Heartbeat;

// TCP连接状态枚举
enum TCPConnectionStatus {
  TCP_DISCONNECTED = 0,
  TCP_CONNECTING = 1,
  TCP_CONNECTED = 2,
  TCP_CONNECTION_FAILED = 3
};

// 数据包头部结构
struct PacketHeader {
  uint16_t length;     // 数据包长度
  uint16_t checksum;   // 校验和
};

class TCPProtocol {
public:
  TCPProtocol();
  ~TCPProtocol();
// 初始化TCP协议
bool begin(Device* device, RS485* rs485, MDNSService* mdnsService);  // 修改函数签名，添加mDNS服务参数


  // 处理TCP连接和数据传输
  void handle();

  // 发送数据到对端设备
  bool sendData(const uint8_t* data, size_t length);

  // 检查是否有数据可读
  bool available();

  // 接收数据
  int receiveData(uint8_t* buffer, size_t bufferSize);

  // 获取连接状态
  TCPConnectionStatus getConnectionStatus();

  // 获取连接状态字符串
  String getConnectionStatusString();
  
  // 设置连接状态回调函数
  typedef void (*ConnectionStatusCallback)(TCPConnectionStatus status);
  void setConnectionStatusCallback(ConnectionStatusCallback callback);
  
  // 设置心跳模块
  void setHeartbeat(Heartbeat* heartbeat);

private:
Device* device;
RS485* rs485;
MDNSService* mdnsService;  // 添加mDNS服务指针
Heartbeat* heartbeat;      // 添加心跳模块指针

WiFiServer* server;
WiFiClient client;

TCPConnectionStatus connectionStatus;
unsigned long lastConnectionAttempt;
unsigned long connectionStartTime;

ConnectionStatusCallback statusCallback;

// 接收缓冲区
static const size_t RECEIVE_BUFFER_SIZE = 1024;
uint8_t receiveBuffer[RECEIVE_BUFFER_SIZE];
size_t receiveBufferIndex;

// 不完整数据包缓冲区
static const size_t INCOMPLETE_PACKET_BUFFER_SIZE = 1024;
uint8_t incompletePacketBuffer[INCOMPLETE_PACKET_BUFFER_SIZE];
size_t incompletePacketSize;
PacketHeader incompletePacketHeader;

// 发送缓冲区
static const size_t SEND_BUFFER_SIZE = 1024;
uint8_t sendBuffer[SEND_BUFFER_SIZE];

// 包相关常量
static const uint16_t PACKET_HEADER_SIZE = sizeof(PacketHeader);
static const unsigned long CONNECTION_TIMEOUT = 15000; // 15秒连接超时
static const unsigned long RECONNECT_INTERVAL = 30000; // 30秒重连间隔
static const uint16_t MAX_PACKET_SIZE = 1024;

// 内部辅助函数
void updateConnectionStatus(TCPConnectionStatus status);
bool isConnectionTimedOut();
uint16_t calculateChecksum(const uint8_t* data, size_t length);
bool sendPacket(const uint8_t* data, size_t length);
int receivePacket(uint8_t* buffer, size_t bufferSize);
void handleServer();
void handleClient();
bool connectToMaster();
bool discoverMasterIP(IPAddress& masterIP, uint16_t& masterPort);  // 添加发现主设备IP的函数声明

// 心跳包处理函数
bool isHeartbeatPacket(const uint8_t* data, size_t length);
void handleHeartbeatPacket(const uint8_t* data, size_t length);
};

#endif // TCP_PROTOCOL_H