#pragma once

#include <cstdint>

#include <string>
#include <unordered_map>

#include <Core/Utility/RandomNumberGenerator.h>
#include <Core/Loaders/AbstractLoader.h>

namespace Core {
enum class Type {
    NORMAL,
    ISO,
};

enum AsyncOperationType {
    ASYNC_OPERATION_OPEN,
    ASYNC_OPERATION_READ,
    ASYNC_OPERATION_WRITE,
    ASYNC_OPERATION_CLOSE,
    ASYNC_OPERATION_SEEK,
    ASYNC_OPERATION_IOCTL,
};

struct FileInfo {
    std::string path;
    int flags;
    int permissions;
    size_t size;
};

static constexpr const int fileDescriptorMinRange = 1000;
static constexpr const int fileDescriptorMaxRange = 50000;

static constexpr const int directoryFileDescriptorMinRange = 100;
static constexpr const int directoryFileDescriptorMaxRange = 1000;

struct PSPFilesystem {
private:
    Core::Utility::RandomGenerator fdRNG, dfdRNG;
    Core::Loader::AbstractLoader *_loader;
    std::string cwd;
    std::unordered_map<std::string, int> blockDevices;
private:
    int generateFileDescriptor();
    int generateDirectoryFileDescriptor();
public:
    PSPFilesystem();
    ~PSPFilesystem();

    void reset();
    void setLoader(Core::Loader::AbstractLoader *loader);

    bool createDevice(const std::string& deviceName);
    bool destroyDevice(const std::string& deviceName);
    bool mountDevice(const std::string& deviceName, const std::string& mount);

    int setCurrentDirectory(const std::string& path);
    int createDirectory(const std::string& dir, int mode);
    int removeDirectory(const std::string& dir, int mode);
    bool addFile(const FileInfo *file);
    int renameFile(const std::string& path);
    int removeFile(const std::string& path);

    int devctl(const char *dev, uint32_t command, uint32_t inputAddr, uint32_t inputLength, uint32_t outputAddr, uint32_t outputLength);
    int ioctl(int fd, uint32_t command, uint32_t inputAddr, uint32_t inputLength, uint32_t outputAddr, uint32_t outputLength);

    int open(const char *file, int flags, int mode, bool async = false);
    int read(int fd, uint32_t dataOut, uint32_t size, bool async = false);
    int write(int fd, uint32_t dataIn, uint32_t size, bool async = false);
    int close(int fd, bool async = false); 
    int seek(int fd, int64_t offset, int whence, bool async = false);

    int directoryOpen(const char *directory);
    int directoryRead(int fd, uint32_t dirEntAddr);
    int directoryClose(int fd);

    int getStat(const char *file, uint32_t statPtr);
    int getAsyncStat(int fd, int poll, uint32_t resultAddr);
    int pollAsync(int fd, uint32_t resultAddr);
    int cancelAsync(int fd);
};
}