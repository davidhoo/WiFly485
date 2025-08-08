#ifndef GPIO_CONTROLLER_H
#define GPIO_CONTROLLER_H

#include "hal_interfaces.h"

class GPIOController : public IGPIOController {
private:
    bool initialized;

public:
    GPIOController();
    virtual ~GPIOController() = default;
    
    bool initialize() override;
    void pinMode(uint8_t pin, uint8_t mode) override;
    void digitalWrite(uint8_t pin, uint8_t value) override;
    int digitalRead(uint8_t pin) override;
    int analogRead(uint8_t pin) override;
    void analogWrite(uint8_t pin, int value) override;
    
    bool isInitialized() const { return initialized; }
};

#endif