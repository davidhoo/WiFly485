#include "uart_controller.h"
#include "debug_utils.h"

UARTController::UARTController(HardwareSerial* serialPort) 
    : serial(serialPort), initialized(false), currentBaudRate(0), currentConfig(0) {
}

bool UARTController::initialize(uint32_t baudRate, uint8_t config) {
    DEBUG_INFO_PRINT("Initializing UART Controller...");
    DEBUG_INFO_PRINT("Baud Rate: %d, Config: 0x%02X", baudRate, config);
    
    if (!serial) {
        DEBUG_ERROR_PRINT("Serial port is null");
        return false;
    }
    
    // 如果已经初始化且参数相同，直接返回成功
    if (initialized && currentBaudRate == baudRate && currentConfig == config) {
        DEBUG_INFO_PRINT("UART already initialized with same parameters");
        return true;
    }
    
    // 如果已经初始化但参数不同，先结束当前连接
    if (initialized) {
        serial->end();
    }
    
    // 初始化串口
    serial->begin(baudRate, (SerialConfig)config);
    
    // 等待串口稳定
    delay(100);
    
    currentBaudRate = baudRate;
    currentConfig = config;
    initialized = true;
    
    DEBUG_INFO_PRINT("UART Controller initialized successfully");
    return true;
}

size_t UARTController::write(const uint8_t* data, size_t length) {
    if (!initialized || !serial) {
        DEBUG_ERROR_PRINT("UART Controller not initialized");
        return 0;
    }
    
    size_t written = serial->write(data, length);
    DEBUG_VERBOSE_PRINT("UART wrote %d/%d bytes", written, length);
    
    return written;
}

size_t UARTController::available() {
    if (!initialized || !serial) {
        return 0;
    }
    
    return serial->available();
}

int UARTController::read() {
    if (!initialized || !serial) {
        DEBUG_ERROR_PRINT("UART Controller not initialized");
        return -1;
    }
    
    return serial->read();
}

size_t UARTController::readBytes(uint8_t* buffer, size_t length) {
    if (!initialized || !serial) {
        DEBUG_ERROR_PRINT("UART Controller not initialized");
        return 0;
    }
    
    size_t bytesRead = serial->readBytes(buffer, length);
    DEBUG_VERBOSE_PRINT("UART read %d bytes", bytesRead);
    
    return bytesRead;
}

void UARTController::flush() {
    if (!initialized || !serial) {
        DEBUG_ERROR_PRINT("UART Controller not initialized");
        return;
    }
    
    serial->flush();
    DEBUG_VERBOSE_PRINT("UART flushed");
}

void UARTController::end() {
    if (!initialized || !serial) {
        DEBUG_WARNING_PRINT("UART Controller not initialized");
        return;
    }
    
    serial->end();
    initialized = false;
    currentBaudRate = 0;
    currentConfig = 0;
    
    DEBUG_INFO_PRINT("UART Controller ended");
}