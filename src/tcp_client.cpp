#include "tcp_client.h"
#include "logger.h"
#include "error_handler.h"
#include "config.h"

TCPClient::TCPClient() : device(nullptr),
                         mdnsService(nullptr),
                         connectionStatus(TCP_DISCONNECTED),
                         masterPort(0),
                         statusCallback(nullptr),
                         dataCallback(nullptr),
                         lastDiscoveryTime(0),
                         lastConnectionAttempt(0),
                         lastHeartbeat(0),
                         connectionRetryCount(0)
{
  // 构造函数
}

TCPClient::~TCPClient()
{
  // 析构函数
  stopConnection();
}

bool TCPClient::begin(Device* device, MDNSService* mdnsService)
{
  this->device = device;
  this->mdnsService = mdnsService;
  
  if (!this->device || !this->mdnsService)
  {
    REPORT_ERROR(ERROR_INVALID_PARAMETER, "TCPClient", "Invalid device or mdnsService");
    return false;
  }
  
  // 只有从设备才需要TCP客户端功能
  if (!device->isSlave())
  {
    LOG_W("TCPClient", "TCP client is only for slave devices");
    return false;
  }
  
  LOG_I("TCPClient", "TCP client initialized for slave device");
  return true;
}

bool TCPClient::startConnection()
{
  if (!device || !mdnsService)
  {
    REPORT_ERROR(ERROR_INVALID_PARAMETER, "TCPClient", "Not initialized");
    return false;
  }
  
  if (!device->isSlave())
  {
    LOG_W("TCPClient", "TCP client connection is only for slave devices");
    return false;
  }
  
  LOG_I("TCPClient", "Starting TCP connection to master device");
  updateConnectionStatus(TCP_DISCOVERING);
  
  // 重置连接参数
  connectionRetryCount = 0;
  lastDiscoveryTime = 0;
  lastConnectionAttempt = 0;
  
  return true;
}

void TCPClient::stopConnection()
{
  if (wifiClient.connected())
  {
    wifiClient.stop();
  }
  
  updateConnectionStatus(TCP_DISCONNECTED);
  masterIP = "";
  masterPort = 0;
  connectionRetryCount = 0;
  
  LOG_I("TCPClient", "TCP connection stopped");
}

void TCPClient::handle()
{
  if (!device || !device->isSlave())
  {
    return;
  }
  
  unsigned long currentTime = millis();
  
  switch (connectionStatus)
  {
    case TCP_DISCOVERING:
      // 定期尝试发现主设备
      if (currentTime - lastDiscoveryTime >= TCP_DISCOVERY_INTERVAL)
      {
        if (discoverMaster())
        {
          updateConnectionStatus(TCP_CONNECTING);
          lastConnectionAttempt = currentTime;
        }
        lastDiscoveryTime = currentTime;
      }
      break;
      
    case TCP_CONNECTING:
      // 尝试连接到主设备
      if (currentTime - lastConnectionAttempt >= TCP_CONNECTION_TIMEOUT)
      {
        // 连接超时
        connectionRetryCount++;
        if (connectionRetryCount >= TCP_MAX_RETRY_COUNT)
        {
          LOG_E("TCPClient", "Max retry count reached, switching to discovery mode");
          updateConnectionStatus(TCP_DISCOVERING);
          connectionRetryCount = 0;
        }
        else
        {
          LOG_W("TCPClient", "Connection timeout, retrying... (%d/%d)", 
                connectionRetryCount, TCP_MAX_RETRY_COUNT);
          lastConnectionAttempt = currentTime + TCP_RETRY_DELAY;
        }
      }
      else if (!wifiClient.connected())
      {
        // 尝试连接
        if (connectToMaster())
        {
          updateConnectionStatus(TCP_CONNECTED);
          lastHeartbeat = currentTime;
          connectionRetryCount = 0;
          LOG_I("TCPClient", "Successfully connected to master at %s:%d", 
                masterIP.c_str(), masterPort);
        }
      }
      break;
      
    case TCP_CONNECTED:
      // 处理已连接状态
      if (!isConnectionAlive())
      {
        LOG_W("TCPClient", "Connection lost, attempting to reconnect");
        resetConnection();
        updateConnectionStatus(TCP_DISCOVERING);
      }
      else
      {
        // 处理数据接收
        handleDataReceive();
        
        // 处理心跳
        handleHeartbeat();
      }
      break;
      
    case TCP_ERROR:
      // 错误状态，等待一段时间后重新开始发现
      if (currentTime - lastConnectionAttempt >= TCP_RETRY_DELAY * 2)
      {
        LOG_I("TCPClient", "Recovering from error, restarting discovery");
        updateConnectionStatus(TCP_DISCOVERING);
        connectionRetryCount = 0;
      }
      break;
      
    default:
      break;
  }
}

TCPConnectionStatus TCPClient::getConnectionStatus()
{
  return connectionStatus;
}

String TCPClient::getConnectionStatusString()
{
  switch (connectionStatus)
  {
    case TCP_DISCONNECTED:
      return "Disconnected";
    case TCP_DISCOVERING:
      return "Discovering";
    case TCP_CONNECTING:
      return "Connecting";
    case TCP_CONNECTED:
      return "Connected";
    case TCP_ERROR:
      return "Error";
    default:
      return "Unknown";
  }
}

bool TCPClient::sendData(const uint8_t* data, size_t length)
{
  if (connectionStatus != TCP_CONNECTED || !wifiClient.connected())
  {
    LOG_W("TCPClient", "Cannot send data: not connected");
    return false;
  }
  
  if (!data || length == 0)
  {
    LOG_W("TCPClient", "Invalid data parameters");
    return false;
  }
  
  size_t bytesWritten = wifiClient.write(data, length);
  if (bytesWritten != length)
  {
    LOG_E("TCPClient", "Failed to send all data: %d/%d bytes sent", bytesWritten, length);
    return false;
  }
  
  LOG_D("TCPClient", "Sent %d bytes to master", length);
  return true;
}

int TCPClient::available()
{
  if (connectionStatus == TCP_CONNECTED && wifiClient.connected())
  {
    return wifiClient.available();
  }
  return 0;
}

int TCPClient::read()
{
  if (connectionStatus == TCP_CONNECTED && wifiClient.connected())
  {
    return wifiClient.read();
  }
  return -1;
}

int TCPClient::read(uint8_t* buffer, size_t size)
{
  if (connectionStatus == TCP_CONNECTED && wifiClient.connected())
  {
    return wifiClient.read(buffer, size);
  }
  return 0;
}

String TCPClient::getMasterIP()
{
  return masterIP;
}

uint16_t TCPClient::getMasterPort()
{
  return masterPort;
}

void TCPClient::setConnectionStatusCallback(ConnectionStatusCallback callback)
{
  statusCallback = callback;
}

void TCPClient::setDataReceivedCallback(DataReceivedCallback callback)
{
  dataCallback = callback;
}

void TCPClient::updateConnectionStatus(TCPConnectionStatus status)
{
  if (connectionStatus != status)
  {
    connectionStatus = status;
    
    // 调用回调函数
    if (statusCallback)
    {
      statusCallback(status);
    }
    
    LOG_I("TCPClient", "Connection status changed to %s", getConnectionStatusString().c_str());
  }
}

bool TCPClient::discoverMaster()
{
  String discoveredIP;
  uint16_t discoveredPort;
  
  if (mdnsService->discoverMaster(discoveredIP, discoveredPort))
  {
    masterIP = discoveredIP;
    masterPort = discoveredPort;
    LOG_I("TCPClient", "Discovered master at %s:%d", masterIP.c_str(), masterPort);
    return true;
  }
  
  LOG_D("TCPClient", "No master device found via mDNS");
  return false;
}

bool TCPClient::connectToMaster()
{
  if (masterIP.length() == 0 || masterPort == 0)
  {
    LOG_E("TCPClient", "Invalid master address");
    return false;
  }
  
  LOG_I("TCPClient", "Attempting to connect to master at %s:%d", masterIP.c_str(), masterPort);
  
  if (wifiClient.connect(masterIP.c_str(), masterPort))
  {
    LOG_I("TCPClient", "TCP connection established");
    return true;
  }
  else
  {
    LOG_W("TCPClient", "Failed to connect to master");
    return false;
  }
}

void TCPClient::handleDataReceive()
{
  int availableBytes = wifiClient.available();
  if (availableBytes > 0)
  {
    // 限制读取的数据量
    int bytesToRead = min(availableBytes, (int)sizeof(receiveBuffer));
    int bytesRead = wifiClient.read(receiveBuffer, bytesToRead);
    
    if (bytesRead > 0)
    {
      LOG_D("TCPClient", "Received %d bytes from master", bytesRead);
      
      // 调用数据接收回调函数
      if (dataCallback)
      {
        dataCallback(receiveBuffer, bytesRead);
      }
    }
  }
}

void TCPClient::handleHeartbeat()
{
  unsigned long currentTime = millis();
  
  // 定期发送心跳包
  if (currentTime - lastHeartbeat >= TCP_HEARTBEAT_INTERVAL)
  {
    // 发送简单的心跳数据
    const char* heartbeat = "HEARTBEAT";
    if (sendData((const uint8_t*)heartbeat, strlen(heartbeat)))
    {
      lastHeartbeat = currentTime;
      LOG_D("TCPClient", "Heartbeat sent");
    }
    else
    {
      LOG_W("TCPClient", "Failed to send heartbeat");
    }
  }
}

bool TCPClient::isConnectionAlive()
{
  return wifiClient.connected();
}

void TCPClient::resetConnection()
{
  if (wifiClient.connected())
  {
    wifiClient.stop();
  }
  
  masterIP = "";
  masterPort = 0;
  connectionRetryCount = 0;
}