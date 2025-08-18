#include "rs485.h"
#include <Arduino.h>

// 构造函数，初始化引脚配置
RS485::RS485(uint8_t rtsPin, uint8_t rxPin, uint8_t txPin) 
    : _rtsPin(rtsPin), _rxPin(rxPin), _txPin(txPin), _baudRate(9600), _errorStatus(ERROR_NONE) {
}

// 初始化RS485通信
bool RS485::begin(unsigned long baudRate) {
    _baudRate = baudRate;
    
    // 初始化RTS引脚为输出
    pinMode(_rtsPin, OUTPUT);
    // 默认设置为接收模式
    setDirection(false);
    
    // 初始化串口通信，使用Serial1而不是Serial
    // 这样可以避免与测试运行器使用的Serial冲突
    Serial1.begin(_baudRate);
    
    return true;
}

// 发送数据
bool RS485::send(const uint8_t* data, size_t length) {
    if (data == nullptr || length == 0) {
        _errorStatus |= ERROR_INVALID_DATA;
        return false;
    }
    
    // 设置为发送模式
    setDirection(true);
    
    // 等待一段时间确保方向切换完成
    delay(1);
    
    // 发送数据
    size_t sent = Serial1.write(data, length);
    
    // 等待发送完成
    Serial1.flush();
    
    // 切换回接收模式
    setDirection(false);
    
    // 检查是否所有数据都已发送
    if (sent != length) {
        _errorStatus |= ERROR_COMMUNICATION_FAILURE;
        return false;
    }
    
    return true;
}

// 接收数据
int RS485::receive(uint8_t* buffer, size_t bufferSize) {
    if (buffer == nullptr || bufferSize == 0) {
        _errorStatus |= ERROR_INVALID_DATA;
        return -1;
    }
    
    size_t bytesRead = 0;
    
    // 从串口读取数据
    while (Serial1.available() && bytesRead < bufferSize) {
        buffer[bytesRead] = Serial1.read();
        bytesRead++;
    }
    
    // 如果缓冲区满了但还有数据，标记缓冲区溢出错误
    if (Serial1.available() && bytesRead == bufferSize) {
        _errorStatus |= ERROR_BUFFER_OVERFLOW;
    }
    
    return bytesRead;
}

// 检查是否有数据可读
bool RS485::available() {
    return Serial1.available() > 0;
}

// 设置通信方向（true为发送，false为接收）
void RS485::setDirection(bool transmit) {
    digitalWrite(_rtsPin, transmit ? HIGH : LOW);
}

// 获取错误状态
uint8_t RS485::getErrorStatus() {
    return _errorStatus;
}

// 清除错误状态
void RS485::clearErrorStatus() {
    _errorStatus = ERROR_NONE;
}

// 获取缓冲区使用情况
size_t RS485::getBufferUsage() {
    return _receiveBuffer.size();
}