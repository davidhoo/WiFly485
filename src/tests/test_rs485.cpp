#include "test_framework.h"
#include "rs485.h"
#include <Arduino.h>

static RS485* testRS485 = nullptr;

// 测试RS485初始化功能
void testRS485Initialization() {
    Serial.println("Testing RS485 Initialization...");
    
    // 创建RS485实例
    testRS485 = new RS485();
    
    // 初始化RS485通信
    bool result = testRS485->begin(9600);
    
    // 验证初始化结果
    if (result) {
        Serial.println("  RS485 initialization successful");
    } else {
        Serial.println("  RS485 initialization failed");
    }
    
    Serial.println("RS485 Initialization test completed!");
}

// 测试半双工方向控制功能
void testRS485DirectionControl() {
    Serial.println("Testing RS485 Direction Control...");
    
    if (testRS485) {
        // 测试发送模式
        testRS485->setDirection(true);
        delay(10);
        
        // 测试接收模式
        testRS485->setDirection(false);
        delay(10);
        
        Serial.println("  RS485 direction control test completed");
        Serial.println("RS485 Direction Control test completed!");
    } else {
        Serial.println("  RS485 not initialized");
        Serial.println("RS485 Direction Control test completed!");
    }
}

// 测试数据发送功能
void testRS485Send() {
    Serial.println("Testing RS485 Send...");
    
    if (testRS485) {
        // 准备测试数据
        const char* testData = "Hello RS485";
        size_t dataLength = strlen(testData);
        
        // 发送数据
        bool result = testRS485->send((const uint8_t*)testData, dataLength);
        
        // 验证发送结果
        if (result) {
            Serial.println("  RS485 data send successful");
        } else {
            Serial.println("  RS485 data send failed");
        }
        
        // 检查错误状态
        uint8_t errorStatus = testRS485->getErrorStatus();
        if (errorStatus == 0) {
            Serial.println("  No errors after send operation");
        } else {
            Serial.printf("  Error detected after send operation: %d\n", errorStatus);
        }
        
        Serial.println("RS485 Send test completed!");
    } else {
        Serial.println("  RS485 not initialized");
        Serial.println("RS485 Send test completed!");
    }
}

// 测试数据接收功能
void testRS485Receive() {
    Serial.println("Testing RS485 Receive...");
    
    if (testRS485) {
        // 准备接收缓冲区
        uint8_t buffer[32];
        
        // 尝试接收数据（在实际测试中可能需要外部设备发送数据）
        int bytesRead = testRS485->receive(buffer, sizeof(buffer));
        
        // 验证接收结果
        if (bytesRead >= 0) {
            Serial.println("  RS485 data receive test completed");
        } else {
            Serial.println("  RS485 data receive failed");
        }
        
        // 检查是否有数据可读
        bool dataAvailable = testRS485->available();
        Serial.println("  RS485 availability check completed");
        
        Serial.println("RS485 Receive test completed!");
    } else {
        Serial.println("  RS485 not initialized");
        Serial.println("RS485 Receive test completed!");
    }
}

// 测试错误处理功能
void testRS485ErrorHandling() {
    Serial.println("Testing RS485 Error Handling...");
    
    if (testRS485) {
        // 测试无效数据发送
        bool result = testRS485->send(nullptr, 0);
        
        // 验证发送结果
        if (!result) {
            Serial.println("  Invalid data send correctly failed");
        } else {
            Serial.println("  Invalid data send should have failed");
        }
        
        // 验证错误状态
        uint8_t errorStatus = testRS485->getErrorStatus();
        if (errorStatus & 4) { // ERROR_INVALID_DATA
            Serial.println("  Invalid data error handling works");
        } else {
            Serial.println("  Invalid data error not detected");
        }
        
        // 清除错误状态
        testRS485->clearErrorStatus();
        
        // 验证错误状态已清除
        errorStatus = testRS485->getErrorStatus();
        if (errorStatus == 0) {
            Serial.println("  Error status cleared successfully");
        } else {
            Serial.println("  Error status not cleared");
        }
        
        Serial.println("RS485 Error Handling test completed!");
    } else {
        Serial.println("  RS485 not initialized");
        Serial.println("RS485 Error Handling test completed!");
    }
}

// 测试缓冲区使用情况
void testRS485BufferUsage() {
    Serial.println("Testing RS485 Buffer Usage...");
    
    if (testRS485) {
        // 获取缓冲区使用情况
        size_t bufferUsage = testRS485->getBufferUsage();
        
        Serial.printf("  Buffer usage: %d\n", bufferUsage);
        Serial.println("RS485 Buffer Usage test completed!");
    } else {
        Serial.println("  RS485 not initialized");
        Serial.println("RS485 Buffer Usage test completed!");
    }
}

// 主测试函数
void runRS485Tests() {
    Serial.println("==================================================");
    Serial.println("Running RS485 Tests...");
    Serial.println("==================================================");
    
    // 运行各个测试
    testRS485Initialization();
    testRS485DirectionControl();
    testRS485Send();
    testRS485Receive();
    testRS485ErrorHandling();
    testRS485BufferUsage();
    
    // 清理资源
    if (testRS485) {
        delete testRS485;
        testRS485 = nullptr;
    }
    
    Serial.println("==================================================");
    Serial.println("All RS485 Tests completed!");
    Serial.println("==================================================");
}