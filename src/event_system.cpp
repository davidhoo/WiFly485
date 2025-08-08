#include "event_system.h"

// 静态成员初始化
EventSystem* EventSystem::instance = nullptr;

EventSystem::EventSystem() : initialized(false), maxQueueSize(DEFAULT_MAX_QUEUE_SIZE) {
}

EventSystem::~EventSystem() {
    cleanup();
}

EventSystem* EventSystem::getInstance() {
    if (!instance) {
        instance = new EventSystem();
    }
    return instance;
}

void EventSystem::destroyInstance() {
    if (instance) {
        delete instance;
        instance = nullptr;
    }
}

bool EventSystem::initialize(size_t maxQueue) {
    DEBUG_INFO_PRINT("Initializing Event System...");
    
    maxQueueSize = maxQueue;
    eventQueue.reserve(maxQueueSize);
    
    initialized = true;
    
    DEBUG_INFO_PRINT("Event System initialized (max queue size: %d)", maxQueueSize);
    return true;
}

void EventSystem::cleanup() {
    if (!initialized) {
        return;
    }
    
    DEBUG_INFO_PRINT("Cleaning up Event System...");
    
    removeAllListeners();
    eventQueue.clear();
    
    initialized = false;
    DEBUG_INFO_PRINT("Event System cleaned up");
}

bool EventSystem::addEventListener(EventType type, const String& listenerName, EventHandler handler, bool oneTime) {
    if (!initialized) {
        DEBUG_ERROR_PRINT("Event System not initialized");
        return false;
    }
    
    if (!handler) {
        DEBUG_ERROR_PRINT("Invalid event handler");
        return false;
    }
    
    // 检查是否已存在同名监听器
    auto& typeListeners = listeners[type];
    for (const auto& listener : typeListeners) {
        if (listener.name == listenerName) {
            DEBUG_WARNING_PRINT("Event listener already exists: %s for event %s", 
                               listenerName.c_str(), getEventTypeName(type).c_str());
            return false;
        }
    }
    
    typeListeners.emplace_back(listenerName, handler, oneTime);
    
    DEBUG_VERBOSE_PRINT("Event listener added: %s for event %s", 
                        listenerName.c_str(), getEventTypeName(type).c_str());
    return true;
}

bool EventSystem::removeEventListener(EventType type, const String& listenerName) {
    if (!initialized) {
        DEBUG_ERROR_PRINT("Event System not initialized");
        return false;
    }
    
    auto it = listeners.find(type);
    if (it == listeners.end()) {
        DEBUG_WARNING_PRINT("No listeners found for event type: %s", getEventTypeName(type).c_str());
        return false;
    }
    
    auto& typeListeners = it->second;
    auto listenerIt = std::find_if(typeListeners.begin(), typeListeners.end(),
        [&listenerName](const EventListener& listener) {
            return listener.name == listenerName;
        });
    
    if (listenerIt != typeListeners.end()) {
        typeListeners.erase(listenerIt);
        DEBUG_VERBOSE_PRINT("Event listener removed: %s for event %s", 
                           listenerName.c_str(), getEventTypeName(type).c_str());
        
        // 如果该类型没有监听器了，删除整个条目
        if (typeListeners.empty()) {
            listeners.erase(it);
        }
        
        return true;
    }
    
    DEBUG_WARNING_PRINT("Event listener not found: %s for event %s", 
                       listenerName.c_str(), getEventTypeName(type).c_str());
    return false;
}

void EventSystem::removeAllListeners(EventType type) {
    if (!initialized) {
        return;
    }
    
    auto it = listeners.find(type);
    if (it != listeners.end()) {
        size_t count = it->second.size();
        listeners.erase(it);
        DEBUG_INFO_PRINT("Removed %d listeners for event %s", count, getEventTypeName(type).c_str());
    }
}

void EventSystem::removeAllListeners() {
    if (!initialized) {
        return;
    }
    
    size_t totalCount = getTotalListenerCount();
    listeners.clear();
    DEBUG_INFO_PRINT("Removed all %d event listeners", totalCount);
}

bool EventSystem::publishEvent(EventType type, const String& source, const String& message, void* data, size_t dataSize) {
    if (!initialized) {
        DEBUG_ERROR_PRINT("Event System not initialized");
        return false;
    }
    
    // 检查队列是否已满
    if (eventQueue.size() >= maxQueueSize) {
        DEBUG_WARNING_PRINT("Event queue full, dropping oldest event");
        eventQueue.erase(eventQueue.begin());
    }
    
    eventQueue.emplace_back(type, source, message, data, dataSize);
    
    DEBUG_VERBOSE_PRINT("Event published: %s from %s", 
                       getEventTypeName(type).c_str(), source.c_str());
    return true;
}

bool EventSystem::publishEvent(const EventData& event) {
    return publishEvent(event.type, event.source, event.message, event.data, event.dataSize);
}

void EventSystem::processEvents() {
    if (!initialized || eventQueue.empty()) {
        return;
    }
    
    // 处理所有排队的事件
    auto eventsToProcess = eventQueue;
    eventQueue.clear();
    
    for (const auto& event : eventsToProcess) {
        processEvent(event);
    }
}

void EventSystem::processEvent(const EventData& event) {
    auto it = listeners.find(event.type);
    if (it == listeners.end()) {
        DEBUG_VERBOSE_PRINT("No listeners for event: %s", getEventTypeName(event.type).c_str());
        return;
    }
    
    auto& typeListeners = it->second;
    auto listenerIt = typeListeners.begin();
    
    while (listenerIt != typeListeners.end()) {
        DEBUG_VERBOSE_PRINT("Processing event %s for listener %s",
                           getEventTypeName(event.type).c_str(), listenerIt->name.c_str());
        
        // 直接调用处理器，ESP8266不支持异常处理
        if (listenerIt->handler) {
            listenerIt->handler(event);
        } else {
            DEBUG_ERROR_PRINT("Invalid event handler: %s", listenerIt->name.c_str());
        }
        
        // 如果是一次性监听器，删除它
        if (listenerIt->oneTime) {
            DEBUG_VERBOSE_PRINT("Removing one-time listener: %s", listenerIt->name.c_str());
            listenerIt = typeListeners.erase(listenerIt);
        } else {
            ++listenerIt;
        }
    }
    
    // 如果该类型没有监听器了，删除整个条目
    if (typeListeners.empty()) {
        listeners.erase(it);
    }
}

size_t EventSystem::getListenerCount(EventType type) const {
    auto it = listeners.find(type);
    return (it != listeners.end()) ? it->second.size() : 0;
}

size_t EventSystem::getTotalListenerCount() const {
    size_t total = 0;
    for (const auto& pair : listeners) {
        total += pair.second.size();
    }
    return total;
}

void EventSystem::printListeners() const {
    DEBUG_INFO_PRINT("=== Event Listeners Report ===");
    DEBUG_INFO_PRINT("Total event types: %d", listeners.size());
    DEBUG_INFO_PRINT("Total listeners: %d", getTotalListenerCount());
    
    for (const auto& pair : listeners) {
        DEBUG_INFO_PRINT("Event %s (%d listeners):", 
                         getEventTypeName(pair.first).c_str(), pair.second.size());
        
        for (const auto& listener : pair.second) {
            DEBUG_INFO_PRINT("  - %s %s", 
                           listener.name.c_str(), 
                           listener.oneTime ? "(one-time)" : "");
        }
    }
    DEBUG_INFO_PRINT("==============================");
}

void EventSystem::printEventQueue() const {
    DEBUG_INFO_PRINT("=== Event Queue Report ===");
    DEBUG_INFO_PRINT("Queue size: %d/%d", eventQueue.size(), maxQueueSize);
    
    for (size_t i = 0; i < eventQueue.size(); ++i) {
        const auto& event = eventQueue[i];
        DEBUG_INFO_PRINT("  [%d] %s from %s: %s", 
                         i, 
                         getEventTypeName(event.type).c_str(),
                         event.source.c_str(),
                         event.message.c_str());
    }
    DEBUG_INFO_PRINT("==========================");
}

String EventSystem::getEventTypeName(EventType type) const {
    switch (type) {
        case EVENT_SYSTEM_STARTUP: return "SYSTEM_STARTUP";
        case EVENT_SYSTEM_SHUTDOWN: return "SYSTEM_SHUTDOWN";
        case EVENT_WIFI_CONNECTED: return "WIFI_CONNECTED";
        case EVENT_WIFI_DISCONNECTED: return "WIFI_DISCONNECTED";
        case EVENT_CONFIG_CHANGED: return "CONFIG_CHANGED";
        case EVENT_DATA_RECEIVED: return "DATA_RECEIVED";
        case EVENT_DATA_SENT: return "DATA_SENT";
        case EVENT_ERROR_OCCURRED: return "ERROR_OCCURRED";
        case EVENT_STATUS_UPDATE: return "STATUS_UPDATE";
        default:
            if (type >= EVENT_CUSTOM_BASE) {
                return "CUSTOM_" + String(type - EVENT_CUSTOM_BASE);
            }
            return "UNKNOWN_" + String(type);
    }
}