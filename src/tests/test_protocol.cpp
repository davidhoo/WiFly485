#include "test_framework.h"
#include "tcp_protocol.h"
#include "device.h"
#include "rs485.h"
#include "config_manager.h"
#include <ESP8266WiFi.h>

// 测试用的模拟设备和RS485
static Device* testDevice = nullptr;
static RS485* testRS485 = nullptr;
static TCPProtocol* tcpProtocol = nullptr;

// 连接状态回调函数
static void connectionStatusCallback(TCPConnectionStatus status) {
  Serial.printf("Test: Connection status changed to %s\n", 
                (status == TCP_DISCONNECTED) ? "Disconnected" :
                (status == TCP_CONNECTING) ? "Connecting" :
                (status == TCP_CONNECTED) ? "Connected" :
                (status == TCP_CONNECTION_FAILED) ? "Connection Failed" : "Unknown");
}

// 测试TCP协议初始化
void testTCPProtocolInitialization() {
  Serial.println("Test: TCP Protocol Initialization");
  
  // 创建测试设备（主设备）
  testDevice = new Device();
  testDevice->setRole(DEVICE_ROLE_MASTER_ENUM);
  testDevice->setName("TestMaster");
  
  // 创建测试RS485
  testRS485 = new RS485();
  bool rs485Result = testRS485->begin(9600);
  ASSERT_TRUE(rs485Result);
  
  // 创建TCP协议实例
  tcpProtocol = new TCPProtocol();
  bool result = tcpProtocol->begin(testDevice, testRS485);
  ASSERT_TRUE(result);
  
  // 设置连接状态回调
  tcpProtocol->setConnectionStatusCallback(connectionStatusCallback);
  
  Serial.println("Test: TCP Protocol Initialization completed");
}

// 测试连接状态管理
void testConnectionStatusManagement() {
  Serial.println("Test: Connection Status Management");
  
  ASSERT_TRUE(tcpProtocol != nullptr);
  
  // 检查初始连接状态
  TCPConnectionStatus status = tcpProtocol->getConnectionStatus();
  ASSERT_EQUAL(TCP_DISCONNECTED, status);
  
  // 检查状态字符串
  String statusString = tcpProtocol->getConnectionStatusString();
  ASSERT_STRING_EQUAL("Disconnected", statusString.c_str());
  
  Serial.println("Test: Connection Status Management completed");
}

// 测试数据发送功能
void testSendData() {
  Serial.println("Test: Send Data Functionality");
  
  ASSERT_TRUE(tcpProtocol != nullptr);
  
  // 创建测试数据
  uint8_t testData[] = {0x01, 0x02, 0x03, 0x04, 0x05};
  
  // 测试无效参数
  bool result1 = tcpProtocol->sendData(nullptr, 5);
  ASSERT_TRUE(!result1);
  
  bool result2 = tcpProtocol->sendData(testData, 0);
  ASSERT_TRUE(!result2);
  
  Serial.println("Test: Send Data Functionality completed");
}

// 测试主从设备创建
void testMasterSlaveCreation() {
  Serial.println("Test: Master/Slave Creation");
  
  // 测试主设备创建
  Device masterDevice;
  masterDevice.setRole(DEVICE_ROLE_MASTER_ENUM);
  masterDevice.setName("TestMaster");
  
  ASSERT_TRUE(masterDevice.isMaster());
  ASSERT_TRUE(!masterDevice.isSlave());
  
  // 测试从设备创建
  Device slaveDevice;
  slaveDevice.setRole(DEVICE_ROLE_SLAVE_ENUM);
  slaveDevice.setName("TestSlave");
  
  ASSERT_TRUE(!slaveDevice.isMaster());
  ASSERT_TRUE(slaveDevice.isSlave());
  
  Serial.println("Test: Master/Slave Creation completed");
}

// 测试TCP协议处理
void testTCPProtocolHandling() {
  Serial.println("Test: TCP Protocol Handling");
  
  ASSERT_TRUE(tcpProtocol != nullptr);
  
  // 调用处理函数（不会有任何效果，因为没有实际连接）
  tcpProtocol->handle();
  
  Serial.println("Test: TCP Protocol Handling completed");
}

// 主测试函数
void runProtocolTests() {
  Serial.println("=== TCP Protocol Tests ===");
  
  // 运行所有测试
  testMasterSlaveCreation();
  testTCPProtocolInitialization();
  testConnectionStatusManagement();
  testSendData();
  testTCPProtocolHandling();
  
  // 清理资源
  if (tcpProtocol) {
    delete tcpProtocol;
    tcpProtocol = nullptr;
  }
  
  if (testRS485) {
    delete testRS485;
    testRS485 = nullptr;
  }
  
  if (testDevice) {
    delete testDevice;
    testDevice = nullptr;
  }
  
  Serial.println("=== TCP PROTOCOL TESTS COMPLETED ===");
}

// 注册测试
void test_protocol_setup() {
  testFramework.registerTest(runProtocolTests, "TCP Protocol Tests");
}