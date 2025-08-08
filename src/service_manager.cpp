#include "service_manager.h"

// ServiceManager 实现
ServiceManager::ServiceManager() : initialized(false), lastUpdateTime(0) {
}

ServiceManager::~ServiceManager() {
    stopAllServices();
}

bool ServiceManager::initialize() {
    DEBUG_INFO_PRINT("Initializing Service Manager...");
    
    initialized = true;
    lastUpdateTime = millis();
    
    DEBUG_INFO_PRINT("Service Manager initialized successfully");
    return true;
}

bool ServiceManager::registerService(IService* service) {
    if (!service) {
        DEBUG_ERROR_PRINT("Cannot register null service");
        return false;
    }
    
    // 检查是否已经注册
    for (auto* existingService : services) {
        if (existingService == service) {
            DEBUG_WARNING_PRINT("Service already registered: %s", service->getName().c_str());
            return true;
        }
        if (existingService->getName() == service->getName()) {
            DEBUG_ERROR_PRINT("Service with same name already exists: %s", service->getName().c_str());
            return false;
        }
    }
    
    services.push_back(service);
    DEBUG_INFO_PRINT("Service registered: %s", service->getName().c_str());
    
    return true;
}

bool ServiceManager::unregisterService(IService* service) {
    if (!service) {
        DEBUG_ERROR_PRINT("Cannot unregister null service");
        return false;
    }
    
    auto it = std::find(services.begin(), services.end(), service);
    if (it != services.end()) {
        // 先停止服务
        if (service->getStatus() == SERVICE_RUNNING) {
            service->stop();
        }
        
        services.erase(it);
        DEBUG_INFO_PRINT("Service unregistered: %s", service->getName().c_str());
        return true;
    }
    
    DEBUG_WARNING_PRINT("Service not found for unregistration: %s", service->getName().c_str());
    return false;
}

bool ServiceManager::startAllServices() {
    if (!initialized) {
        DEBUG_ERROR_PRINT("Service Manager not initialized");
        return false;
    }
    
    DEBUG_INFO_PRINT("Starting all services...");
    
    bool allStarted = true;
    for (auto* service : services) {
        DEBUG_INFO_PRINT("Starting service: %s", service->getName().c_str());
        
        if (!service->initialize()) {
            DEBUG_ERROR_PRINT("Failed to initialize service: %s", service->getName().c_str());
            allStarted = false;
            continue;
        }
        
        if (!service->start()) {
            DEBUG_ERROR_PRINT("Failed to start service: %s", service->getName().c_str());
            allStarted = false;
            continue;
        }
        
        DEBUG_INFO_PRINT("Service started successfully: %s", service->getName().c_str());
    }
    
    if (allStarted) {
        DEBUG_INFO_PRINT("All services started successfully");
    } else {
        DEBUG_WARNING_PRINT("Some services failed to start");
    }
    
    return allStarted;
}

bool ServiceManager::stopAllServices() {
    DEBUG_INFO_PRINT("Stopping all services...");
    
    bool allStopped = true;
    // 反向停止服务，确保依赖关系正确处理
    for (auto it = services.rbegin(); it != services.rend(); ++it) {
        auto* service = *it;
        DEBUG_INFO_PRINT("Stopping service: %s", service->getName().c_str());
        
        if (!service->stop()) {
            DEBUG_ERROR_PRINT("Failed to stop service: %s", service->getName().c_str());
            allStopped = false;
        } else {
            DEBUG_INFO_PRINT("Service stopped successfully: %s", service->getName().c_str());
        }
    }
    
    if (allStopped) {
        DEBUG_INFO_PRINT("All services stopped successfully");
    } else {
        DEBUG_WARNING_PRINT("Some services failed to stop");
    }
    
    return allStopped;
}

void ServiceManager::updateAllServices() {
    if (!initialized) {
        return;
    }
    
    for (auto* service : services) {
        if (service->getStatus() == SERVICE_RUNNING) {
            service->update();
        }
    }
}

IService* ServiceManager::getService(const String& name) {
    for (auto* service : services) {
        if (service->getName() == name) {
            return service;
        }
    }
    return nullptr;
}

void ServiceManager::printServiceStatus() {
    DEBUG_INFO_PRINT("=== Service Status Report ===");
    DEBUG_INFO_PRINT("Total services: %d", services.size());
    
    for (auto* service : services) {
        DEBUG_INFO_PRINT("  %s: %s", 
                         service->getName().c_str(), 
                         service->getStatusString().c_str());
    }
    
    DEBUG_INFO_PRINT("Running: %d, Error: %d", 
                     areAllServicesRunning() ? services.size() : 0,
                     hasErrorServices() ? 1 : 0);
    DEBUG_INFO_PRINT("============================");
}

bool ServiceManager::areAllServicesRunning() {
    for (auto* service : services) {
        if (service->getStatus() != SERVICE_RUNNING) {
            return false;
        }
    }
    return true;
}

bool ServiceManager::hasErrorServices() {
    for (auto* service : services) {
        if (service->getStatus() == SERVICE_ERROR) {
            return true;
        }
    }
    return false;
}

void ServiceManager::update() {
    unsigned long now = millis();
    if (now - lastUpdateTime >= UPDATE_INTERVAL) {
        updateAllServices();
        lastUpdateTime = now;
    }
}

// BaseService 实现
BaseService::BaseService(const String& name, unsigned long interval) 
    : serviceName(name), status(SERVICE_STOPPED), lastUpdateTime(0), updateInterval(interval) {
}

bool BaseService::initialize() {
    DEBUG_VERBOSE_PRINT("Initializing service: %s", serviceName.c_str());
    
    status = SERVICE_STARTING;
    bool result = onInitialize();
    
    if (result) {
        DEBUG_VERBOSE_PRINT("Service initialized: %s", serviceName.c_str());
    } else {
        status = SERVICE_ERROR;
        DEBUG_ERROR_PRINT("Service initialization failed: %s", serviceName.c_str());
    }
    
    return result;
}

bool BaseService::start() {
    if (status != SERVICE_STARTING && status != SERVICE_STOPPED) {
        DEBUG_WARNING_PRINT("Service not in correct state to start: %s (status: %s)", 
                           serviceName.c_str(), getStatusString().c_str());
        return false;
    }
    
    DEBUG_VERBOSE_PRINT("Starting service: %s", serviceName.c_str());
    
    status = SERVICE_STARTING;
    bool result = onStart();
    
    if (result) {
        status = SERVICE_RUNNING;
        lastUpdateTime = millis();
        DEBUG_VERBOSE_PRINT("Service started: %s", serviceName.c_str());
    } else {
        status = SERVICE_ERROR;
        DEBUG_ERROR_PRINT("Service start failed: %s", serviceName.c_str());
    }
    
    return result;
}

void BaseService::update() {
    if (status != SERVICE_RUNNING) {
        return;
    }
    
    if (shouldUpdate()) {
        onUpdate();
        lastUpdateTime = millis();
    }
}

bool BaseService::stop() {
    if (status == SERVICE_STOPPED) {
        return true;
    }
    
    DEBUG_VERBOSE_PRINT("Stopping service: %s", serviceName.c_str());
    
    status = SERVICE_STOPPING;
    bool result = onStop();
    
    if (result) {
        status = SERVICE_STOPPED;
        DEBUG_VERBOSE_PRINT("Service stopped: %s", serviceName.c_str());
    } else {
        status = SERVICE_ERROR;
        DEBUG_ERROR_PRINT("Service stop failed: %s", serviceName.c_str());
    }
    
    return result;
}

String BaseService::getStatusString() const {
    switch (status) {
        case SERVICE_STOPPED: return "STOPPED";
        case SERVICE_STARTING: return "STARTING";
        case SERVICE_RUNNING: return "RUNNING";
        case SERVICE_STOPPING: return "STOPPING";
        case SERVICE_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

bool BaseService::shouldUpdate() {
    unsigned long now = millis();
    return (now - lastUpdateTime >= updateInterval);
}