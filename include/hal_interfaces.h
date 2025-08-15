#ifndef HAL_INTERFACES_H
#define HAL_INTERFACES_H

#include <Arduino.h>
#include <functional>

// 数据回调函数类型定义
typedef std::function<void(const uint8_t* data, size_t length)> DataCallback;
typedef std::function<void(bool connected)> ConnectionCallback;
typedef std::function<void(const String& message)> StatusCallback;

// GPIO控制接口
class IGPIOController {
public:
    virtual ~IGPIOController() = default;
    virtual bool initialize() = 0;
    virtual void pinMode(uint8_t pin, uint8_t mode) = 0;
    virtual void digitalWrite(uint8_t pin, uint8_t value) = 0;
    virtual int digitalRead(uint8_t pin) = 0;
    virtual int analogRead(uint8_t pin) = 0;
    virtual void analogWrite(uint8_t pin, int value) = 0;
};

// UART控制接口
class IUARTController {
public:
    virtual ~IUARTController() = default;
    virtual bool initialize(uint32_t baudRate, uint8_t config = SERIAL_8N1) = 0;
    virtual size_t write(const uint8_t* data, size_t length) = 0;
    virtual size_t available() = 0;
    virtual int read() = 0;
    virtual size_t readBytes(uint8_t* buffer, size_t length) = 0;
    virtual void flush() = 0;
    virtual void end() = 0;
};

// WiFi控制接口
class IWiFiController {
public:
    virtual ~IWiFiController() = default;
    virtual bool initialize() = 0;
    virtual bool startAP(const String& ssid, const String& password) = 0;
    virtual bool connectSTA(const String& ssid, const String& password) = 0;
    virtual bool isConnected() = 0;
    virtual String getLocalIP() = 0;
    virtual String getMacAddress() = 0;
    virtual int getRSSI() = 0;
    virtual void disconnect() = 0;
    virtual void onConnectionChanged(ConnectionCallback callback) = 0;
};

// 文件系统接口
class IFileSystemController {
public:
    virtual ~IFileSystemController() = default;
    virtual bool initialize() = 0;
    virtual bool exists(const String& path) = 0;
    virtual bool writeFile(const String& path, const String& content) = 0;
    virtual String readFile(const String& path) = 0;
    virtual bool deleteFile(const String& path) = 0;
    virtual size_t totalBytes() = 0;
    virtual size_t usedBytes() = 0;
    virtual void format() = 0;
};

// LED指示器接口
class ILEDIndicator {
public:
    virtual ~ILEDIndicator() = default;
    virtual bool initialize() = 0;
    virtual void setState(uint8_t state) = 0;
    virtual uint8_t getState() const = 0;
    virtual void update() = 0;
};

#endif