#include <Arduino.h>
#include "filesystem_controller.h"
#include "debug_utils.h"

// 简单的测试框架
class FileSystemControllerTest {
private:
    FileSystemController fsController;
    int testsPassed;
    int testsTotal;
    
public:
    FileSystemControllerTest() : testsPassed(0), testsTotal(0) {}
    
    void runAllTests() {
        DEBUG_INFO_PRINT("=== FileSystemController Test Suite ===");
        
        // 初始化文件系统
        if (!fsController.initialize()) {
            DEBUG_ERROR_PRINT("Failed to initialize FileSystemController for testing");
            return;
        }
        
        // 运行测试
        testInitialization();
        testFileOperations();
        testDirectoryOperations();
        testFileSystemInfo();
        
        // 输出结果
        DEBUG_INFO_PRINT("=== Test Results ===");
        DEBUG_INFO_PRINT("Tests passed: %d/%d", testsPassed, testsTotal);
        if (testsPassed == testsTotal) {
            DEBUG_INFO_PRINT("All tests PASSED!");
        } else {
            DEBUG_ERROR_PRINT("Some tests FAILED!");
        }
    }
    
private:
    void assertTrue(bool condition, const String& testName) {
        testsTotal++;
        if (condition) {
            testsPassed++;
            DEBUG_INFO_PRINT("✓ %s", testName.c_str());
        } else {
            DEBUG_ERROR_PRINT("✗ %s", testName.c_str());
        }
    }
    
    void testInitialization() {
        DEBUG_INFO_PRINT("--- Testing Initialization ---");
        
        assertTrue(fsController.isInitialized(), "FileSystemController is initialized");
        
        // 测试文件系统信息
        size_t totalBytes = fsController.totalBytes();
        size_t usedBytes = fsController.usedBytes();
        assertTrue(totalBytes > 0, "Total bytes should be greater than 0");
        assertTrue(usedBytes >= 0, "Used bytes should be non-negative");
        assertTrue(usedBytes <= totalBytes, "Used bytes should not exceed total bytes");
    }
    
    void testFileOperations() {
        DEBUG_INFO_PRINT("--- Testing File Operations ---");
        
        String testFilePath = "/test_file.txt";
        String testContent = "This is a test file content.\nWith multiple lines.";
        
        // 测试文件写入
        bool writeResult = fsController.writeFile(testFilePath, testContent);
        assertTrue(writeResult, "File write operation successful");
        
        // 测试文件存在性
        bool existsResult = fsController.exists(testFilePath);
        assertTrue(existsResult, "File should exist after writing");
        
        // 测试文件读取
        String readContent = fsController.readFile(testFilePath);
        assertTrue(readContent == testContent, "File content should match");
        
        // 测试文件删除
        bool deleteResult = fsController.deleteFile(testFilePath);
        assertTrue(deleteResult, "File delete operation successful");
        
        // 验证文件已删除
        bool existsAfterDelete = fsController.exists(testFilePath);
        assertTrue(!existsAfterDelete, "File should not exist after deletion");
        
        // 测试删除不存在的文件（应该成功）
        bool deleteNonExistentResult = fsController.deleteFile("/non_existent_file.txt");
        assertTrue(deleteNonExistentResult, "Deleting non-existent file should succeed");
    }
    
    void testDirectoryOperations() {
        DEBUG_INFO_PRINT("--- Testing Directory Operations ---");
        
        String testDirPath = "/test_directory";
        
        // 测试创建目录
        bool createDirResult = fsController.createDirectory(testDirPath);
        assertTrue(createDirResult, "Directory creation successful");
        
        // 验证目录存在
        bool dirExists = fsController.exists(testDirPath);
        assertTrue(dirExists, "Directory should exist after creation");
        
        // 测试在目录中创建文件
        String filePathInDir = testDirPath + "/file_in_dir.txt";
        bool writeFileInDirResult = fsController.writeFile(filePathInDir, "Content in directory");
        assertTrue(writeFileInDirResult, "Writing file in directory successful");
        
        // 验证文件存在
        bool fileInDirExists = fsController.exists(filePathInDir);
        assertTrue(fileInDirExists, "File in directory should exist");
        
        // 测试删除目录中的文件
        bool deleteFileInDirResult = fsController.deleteFile(filePathInDir);
        assertTrue(deleteFileInDirResult, "Deleting file in directory successful");
        
        // 测试删除目录
        bool removeDirResult = fsController.removeDirectory(testDirPath);
        assertTrue(removeDirResult, "Directory removal successful");
        
        // 验证目录已删除
        bool dirExistsAfterRemoval = fsController.exists(testDirPath);
        assertTrue(!dirExistsAfterRemoval, "Directory should not exist after removal");
    }
    
    void testFileSystemInfo() {
        DEBUG_INFO_PRINT("--- Testing File System Information ---");
        
        size_t totalBytes = fsController.totalBytes();
        size_t usedBytes = fsController.usedBytes();
        
        assertTrue(totalBytes > 0, "Total bytes should be greater than 0");
        assertTrue(usedBytes >= 0, "Used bytes should be non-negative");
        assertTrue(usedBytes <= totalBytes, "Used bytes should not exceed total bytes");
        
        // 测试格式化功能（谨慎测试，会清除所有数据）
        DEBUG_INFO_PRINT("Testing format function (this will clear all data)");
        fsController.format();
        assertTrue(fsController.isInitialized(), "FileSystemController should be initialized after format");
        
        // 重新初始化测试
        bool reinitResult = fsController.initialize();
        assertTrue(reinitResult, "FileSystemController re-initialization successful");
    }
};

// 全局测试实例
FileSystemControllerTest* g_test = nullptr;

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    DEBUG_INFO_PRINT("Starting FileSystemController Tests...");
    
    g_test = new FileSystemControllerTest();
    g_test->runAllTests();
    
    DEBUG_INFO_PRINT("Tests completed. System will now halt.");
}

void loop() {
    // 测试完成后停止
    delay(1000);
}