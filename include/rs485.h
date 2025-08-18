#ifndef RS485_H
#define RS485_H

#include <Arduino.h>
#include <queue>

class RS485 {
public:
    // 构造函数，初始化引脚配置
    RS485(uint8_t rtsPin = 4, uint8_t rxPin = 1, uint8_t txPin = 3);
    
    // 初始化RS485通信
    bool begin(unsigned long baudRate = 9600);
    
    // 发送数据
    bool send(const uint8_t* data, size_t length);
    
    // 接收数据
    int receive(uint8_t* buffer, size_t bufferSize);
    
    // 检查是否有数据可读
    bool available();
    
    // 设置通信方向（true为发送，false为接收）
    void setDirection(bool transmit);
    
    // 获取错误状态
    uint8_t getErrorStatus();
    
    // 清除错误状态
    void clearErrorStatus();
    
    // 获取缓冲区使用情况
    size_t getBufferUsage();
    
private:
    uint8_t _rtsPin;
    uint8_t _rxPin;
    uint8_t _txPin;
    unsigned long _baudRate;
    
    // 错误状态标志
    uint8_t _errorStatus;
    
    // 数据缓冲区
    std::queue<uint8_t> _receiveBuffer;
    
    // 缓冲区大小限制
    static const size_t BUFFER_SIZE_LIMIT = 256;
    
    // 错误状态位定义
    static const uint8_t ERROR_NONE = 0;
    static const uint8_t ERROR_BUFFER_OVERFLOW = 1;
    static const uint8_t ERROR_COMMUNICATION_FAILURE = 2;
    static const uint8_t ERROR_INVALID_DATA = 4;
};

#endif // RS485_H