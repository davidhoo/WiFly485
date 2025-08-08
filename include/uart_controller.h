#ifndef UART_CONTROLLER_H
#define UART_CONTROLLER_H

#include "hal_interfaces.h"
#include <HardwareSerial.h>

class UARTController : public IUARTController {
private:
    HardwareSerial* serial;
    bool initialized;
    uint32_t currentBaudRate;
    uint8_t currentConfig;

public:
    UARTController(HardwareSerial* serialPort = &Serial);
    virtual ~UARTController() = default;
    
    bool initialize(uint32_t baudRate, uint8_t config = SERIAL_8N1) override;
    size_t write(const uint8_t* data, size_t length) override;
    size_t available() override;
    int read() override;
    size_t readBytes(uint8_t* buffer, size_t length) override;
    void flush() override;
    void end() override;
    
    bool isInitialized() const { return initialized; }
    uint32_t getBaudRate() const { return currentBaudRate; }
    uint8_t getConfig() const { return currentConfig; }
};

#endif