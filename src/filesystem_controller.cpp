#include "filesystem_controller.h"
#include "debug_utils.h"

FileSystemController::FileSystemController() : initialized(false) {
}

bool FileSystemController::initialize() {
    DEBUG_INFO_PRINT("Initializing File System Controller...");
    
    if (!LittleFS.begin()) {
        DEBUG_ERROR_PRINT("Failed to mount LittleFS");
        DEBUG_INFO_PRINT("Attempting to format LittleFS...");
        
        if (!LittleFS.format()) {
            DEBUG_ERROR_PRINT("Failed to format LittleFS");
            return false;
        }
        
        DEBUG_INFO_PRINT("LittleFS formatted successfully");
        
        if (!LittleFS.begin()) {
            DEBUG_ERROR_PRINT("Failed to mount LittleFS after format");
            return false;
        }
    }
    
    initialized = true;
    
    // 打印文件系统信息
    FSInfo fsInfo;
    LittleFS.info(fsInfo);
    
    DEBUG_INFO_PRINT("File System initialized successfully");
    DEBUG_INFO_PRINT("Total bytes: %d", fsInfo.totalBytes);
    DEBUG_INFO_PRINT("Used bytes: %d", fsInfo.usedBytes);
    DEBUG_INFO_PRINT("Block size: %d", fsInfo.blockSize);
    DEBUG_INFO_PRINT("Page size: %d", fsInfo.pageSize);
    DEBUG_INFO_PRINT("Max open files: %d", fsInfo.maxOpenFiles);
    DEBUG_INFO_PRINT("Max path length: %d", fsInfo.maxPathLength);
    
    return true;
}

bool FileSystemController::exists(const String& path) {
    if (!initialized) {
        DEBUG_ERROR_PRINT("File System Controller not initialized");
        return false;
    }
    
    bool fileExists = LittleFS.exists(path);
    DEBUG_VERBOSE_PRINT("File exists check: %s -> %s", path.c_str(), fileExists ? "true" : "false");
    
    return fileExists;
}

bool FileSystemController::writeFile(const String& path, const String& content) {
    if (!initialized) {
        DEBUG_ERROR_PRINT("File System Controller not initialized");
        return false;
    }
    
    DEBUG_VERBOSE_PRINT("Writing file: %s (%d bytes)", path.c_str(), content.length());
    
    File file = LittleFS.open(path, "w");
    if (!file) {
        DEBUG_ERROR_PRINT("Failed to open file for writing: %s", path.c_str());
        return false;
    }
    
    size_t bytesWritten = file.print(content);
    file.close();
    
    bool success = (bytesWritten == content.length());
    if (success) {
        DEBUG_VERBOSE_PRINT("File written successfully: %s (%d bytes)", path.c_str(), bytesWritten);
    } else {
        DEBUG_ERROR_PRINT("Failed to write complete file: %s (wrote %d/%d bytes)", 
                          path.c_str(), bytesWritten, content.length());
    }
    
    return success;
}

String FileSystemController::readFile(const String& path) {
    if (!initialized) {
        DEBUG_ERROR_PRINT("File System Controller not initialized");
        return "";
    }
    
    DEBUG_VERBOSE_PRINT("Reading file: %s", path.c_str());
    
    File file = LittleFS.open(path, "r");
    if (!file) {
        DEBUG_ERROR_PRINT("Failed to open file for reading: %s", path.c_str());
        return "";
    }
    
    String content = file.readString();
    file.close();
    
    DEBUG_VERBOSE_PRINT("File read successfully: %s (%d bytes)", path.c_str(), content.length());
    
    return content;
}

bool FileSystemController::deleteFile(const String& path) {
    if (!initialized) {
        DEBUG_ERROR_PRINT("File System Controller not initialized");
        return false;
    }
    
    DEBUG_VERBOSE_PRINT("Deleting file: %s", path.c_str());
    
    if (!LittleFS.exists(path)) {
        DEBUG_WARNING_PRINT("File does not exist: %s", path.c_str());
        return true; // 文件不存在也算删除成功
    }
    
    bool success = LittleFS.remove(path);
    if (success) {
        DEBUG_VERBOSE_PRINT("File deleted successfully: %s", path.c_str());
    } else {
        DEBUG_ERROR_PRINT("Failed to delete file: %s", path.c_str());
    }
    
    return success;
}

size_t FileSystemController::totalBytes() {
    if (!initialized) {
        DEBUG_ERROR_PRINT("File System Controller not initialized");
        return 0;
    }
    
    FSInfo fsInfo;
    LittleFS.info(fsInfo);
    return fsInfo.totalBytes;
}

size_t FileSystemController::usedBytes() {
    if (!initialized) {
        DEBUG_ERROR_PRINT("File System Controller not initialized");
        return 0;
    }
    
    FSInfo fsInfo;
    LittleFS.info(fsInfo);
    return fsInfo.usedBytes;
}

void FileSystemController::format() {
    DEBUG_WARNING_PRINT("Formatting file system...");
    
    if (initialized) {
        LittleFS.end();
        initialized = false;
    }
    
    if (LittleFS.format()) {
        DEBUG_INFO_PRINT("File system formatted successfully");
        initialize(); // 重新初始化
    } else {
        DEBUG_ERROR_PRINT("Failed to format file system");
    }
}

bool FileSystemController::createDirectory(const String& path) {
    if (!initialized) {
        DEBUG_ERROR_PRINT("File System Controller not initialized");
        return false;
    }
    
    DEBUG_VERBOSE_PRINT("Creating directory: %s", path.c_str());
    
    bool success = LittleFS.mkdir(path);
    if (success) {
        DEBUG_VERBOSE_PRINT("Directory created successfully: %s", path.c_str());
    } else {
        DEBUG_ERROR_PRINT("Failed to create directory: %s", path.c_str());
    }
    
    return success;
}

bool FileSystemController::removeDirectory(const String& path) {
    if (!initialized) {
        DEBUG_ERROR_PRINT("File System Controller not initialized");
        return false;
    }
    
    DEBUG_VERBOSE_PRINT("Removing directory: %s", path.c_str());
    
    bool success = LittleFS.rmdir(path);
    if (success) {
        DEBUG_VERBOSE_PRINT("Directory removed successfully: %s", path.c_str());
    } else {
        DEBUG_ERROR_PRINT("Failed to remove directory: %s", path.c_str());
    }
    
    return success;
}

void FileSystemController::listFiles(const String& path) {
    if (!initialized) {
        DEBUG_ERROR_PRINT("File System Controller not initialized");
        return;
    }
    
    DEBUG_INFO_PRINT("Listing files in: %s", path.c_str());
    
    Dir dir = LittleFS.openDir(path);
    int fileCount = 0;
    
    while (dir.next()) {
        fileCount++;
        String fileName = dir.fileName();
        size_t fileSize = dir.fileSize();
        
        DEBUG_INFO_PRINT("  %s (%d bytes)", fileName.c_str(), fileSize);
    }
    
    if (fileCount == 0) {
        DEBUG_INFO_PRINT("  No files found");
    } else {
        DEBUG_INFO_PRINT("Total files: %d", fileCount);
    }
}