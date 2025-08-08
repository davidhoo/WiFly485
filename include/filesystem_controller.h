#ifndef FILESYSTEM_CONTROLLER_H
#define FILESYSTEM_CONTROLLER_H

#include "hal_interfaces.h"
#include <LittleFS.h>

class FileSystemController : public IFileSystemController {
private:
    bool initialized;
    
public:
    FileSystemController();
    virtual ~FileSystemController() = default;
    
    bool initialize() override;
    bool exists(const String& path) override;
    bool writeFile(const String& path, const String& content) override;
    String readFile(const String& path) override;
    bool deleteFile(const String& path) override;
    size_t totalBytes() override;
    size_t usedBytes() override;
    void format() override;
    
    bool isInitialized() const { return initialized; }
    
    // 额外的实用方法
    bool createDirectory(const String& path);
    bool removeDirectory(const String& path);
    void listFiles(const String& path = "/");
};

#endif