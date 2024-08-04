#include <Core/Filesystem/PSPFilesystem.h>
#include <Core/Filesystem/BlockDeviceTree.h>

namespace Core {
PSPFilesystem::PSPFilesystem() {
    fdRNG.setSeed();
    fdRNG.setRange(fileDescriptorMinRange, fileDescriptorMaxRange);

    dfdRNG.setSeed();
    fdRNG.setRange(directoryFileDescriptorMinRange, directoryFileDescriptorMaxRange);
    _loader = nullptr;
}

PSPFilesystem::~PSPFilesystem() {

}

void PSPFilesystem::setLoader(Core::Loader::AbstractLoader *loader) {
    _loader = loader;
}

int PSPFilesystem::generateFileDescriptor() {
    return fdRNG();
}

int PSPFilesystem::generateDirectoryFileDescriptor() {
    return dfdRNG();
}

bool PSPFilesystem::createDevice(const std::string& deviceName) {
    return false;
}

bool PSPFilesystem::mountDevice(const std::string& deviceName, const std::string& mount) {
    return false;
}

bool PSPFilesystem::destroyDevice(const std::string& deviceName) {
    return false;
}

int PSPFilesystem::setCurrentDirectory(const std::string& path) {
    return -1;
}

int PSPFilesystem::createDirectory(const std::string& dir, int mode) {
    return -1;
}

int PSPFilesystem::removeDirectory(const std::string& dir, int mode) {
    return -1;
}

bool PSPFilesystem::addFile(const FileInfo *file) {
    return false;
}

int PSPFilesystem::renameFile(const std::string& path) {
    return -1;
}

int PSPFilesystem::removeFile(const std::string& path) {
    return -1;
}

int PSPFilesystem::devctl(const char *dev, uint32_t command, uint32_t inputAddr, uint32_t inputLength, uint32_t outputAddr, uint32_t outputLength) {
    return -1;
}

int PSPFilesystem::ioctl(int fd, uint32_t command, uint32_t inputAddr, uint32_t inputLength, uint32_t outputAddr, uint32_t outputLength) {
    return -1;
}

int PSPFilesystem::open(const char *file, int flags, int mode, bool async) {
    return -1;
}

int PSPFilesystem::read(int fd, uint32_t dataOut, uint32_t size, bool async) {
    return -1;
}

int PSPFilesystem::write(int fd, uint32_t dataIn, uint32_t size, bool async) {
    return -1;
}

int PSPFilesystem::close(int fd, bool async) {
    return -1;
}

int PSPFilesystem::seek(int fd, int64_t offset, int whence, bool async) {
    return -1;
}

int PSPFilesystem::directoryOpen(const char *directory) {
    return -1;
}

int PSPFilesystem::directoryRead(int fd, uint32_t dirEntAddr) {
    return -1;
}

int PSPFilesystem::directoryClose(int fd) {
    return -1;
}
}