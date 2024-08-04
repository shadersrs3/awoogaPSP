#if defined (_WIN32)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <unordered_map>

#include <Core/HLE/Modules/IoFileMgrForUser.h>

#include <Core/Kernel/sceKernelError.h>
#include <Core/Kernel/sceKernelCallback.h>
#include <Core/Kernel/sceKernelThread.h>
#include <Core/Memory/MemoryAccess.h>
#include <Core/Utility/RandomNumberGenerator.h>

#include <Core/Logger.h>

#include <Core/Loaders/AbstractLoader.h>
#include <Core/Loaders/ISOLoader.h>

#include <deque>

using namespace Core::Kernel;

namespace Core::HLE {
static const char *logType = "IoFileMgrForUser";

std::string hostCurrentDirectory;

struct File {
    std::string hostDirectory;
    int flags;
    SceMode mode;
    int64_t offset;
    int64_t size;
    Core::Loader::ISOFileInfo info;
};

std::unordered_map<int, File> fileMap;

static std::string cwd;

std::shared_ptr<FILE> p = std::shared_ptr<FILE>(fopen("test.txt", "w"), [](FILE *f) { if (f) { fclose(f); } });

void setHostCurrentWorkingDirectory(const std::string& path) {
    hostCurrentDirectory = path;
}

static std::string removeBlockDevice(std::string block) {
    if (auto pos = block.find(':'); pos != -1) {
        if (auto pos2 = block.find(":/"); pos2 != -1) {
            block = block.substr(pos2 + 2);
        } else {
            if (auto pos3 = block.find(':'); pos3 != -1) {
                block = block.substr(pos3 + 1);
            }
        }
    }
    return block;
}

static std::string getHostDirectoryFromFile(std::string guestFile) {
    guestFile = removeBlockDevice(guestFile);
    return hostCurrentDirectory + guestFile;
}

SceUID sceIoOpen(const char *file, int flags, SceMode mode) {
    int fd;

    if (!file || std::string(file) == "") {
        LOG_ERROR(logType, "can't open file while name is NULL flags %08x mode 0x%08x", file, flags, mode);
        return SCE_KERNEL_ERROR_ERRNO_FILE_NOT_FOUND;
    }

    File f {};
    if (auto loader = reinterpret_cast<Core::Loader::ISOLoader *>(Core::Loader::getGameLoaderHandle()); loader && loader->getLoaderType() == Core::Loader::LOADER_TYPE_ISO) {
        auto s = std::string(file);

        std::string path;

        if (s == "umd0:" || s == "umd1:") {
            path = "";
        } else if (!s.starts_with("disc0:")) {
            s = removeBlockDevice(cwd) + "/" + s;
            path = s;
        } else {
            path = removeBlockDevice(s);
        }

        int lbn = 0, size = 0;
        if (path.starts_with("sce_lbn")) {
            (void)sscanf(path.c_str(), "sce_lbn0x%x_size0x%x", &lbn, &size);
            
            f.hostDirectory = path;
            f.size = size;
            f.offset = 0;
            f.flags = flags;
            f.mode = mode;
            f.info = {};
            f.info.logicalBlockAddress = lbn;
        } else if (path == "") {
            f.hostDirectory = path;
            f.size = loader->getFileSize() >> 11;
            f.offset = 0;
            f.flags = flags;
            f.mode = mode;
            f.info = {};
            f.info.logicalBlockAddress = -1;
        } else {
            Core::Loader::ISOFileInfo info;
            bool state = loader->getFileInfo(path, &info);

            if (!state) {
                LOG_ERROR(logType, "can't open file from ISO \"%s\"", file);
                return SCE_KERNEL_ERROR_ERRNO_FILE_NOT_FOUND;
            }

            f.hostDirectory = info.path;
            f.size = info.size;
            f.offset = 0;
            f.flags = flags;
            f.mode = mode;
            f.info = info;
        }
    } else {
        f.hostDirectory = getHostDirectoryFromFile(file);

        FILE *fp;

        fp = fopen(f.hostDirectory.c_str(), "rb");
        if (!fp) {
            LOG_ERROR(logType, "can't open file %s", file);
            return SCE_KERNEL_ERROR_ERRNO_FILE_NOT_FOUND;
        }

        fseek(fp, 0, SEEK_END);
        f.size = ftell(fp);
        fclose(fp);
        f.offset = 0;
        f.flags = flags;
        f.mode = mode;
        f.info = {};
    }

    std::unordered_map<int, File>::iterator p;
    do {
        fd = Core::Utility::getRandomInt();
        p = fileMap.find(fd);
    } while (p != fileMap.end());

    fileMap[fd] = f;

    LOG_SYSCALL(logType, "sceIoOpen(%s, 0x%08x, 0x%08x) fd = 0x%06x", file, flags, mode, fd);
    return fd;
}

int __hleIoRead(SceUID fd, uint32_t data, SceSize size) {
    auto it = fileMap.find(fd);
    if (it == fileMap.end())
        return SCE_KERNEL_ERROR_BADF;

    if (!Core::Memory::Utility::isValidAddress(data))
        return SCE_KERNEL_ERROR_ILLEGAL_ADDR;

    int newSize = size;

    if (int64_t p = it->second.offset + size; p >= it->second.size) {
        if (p - size >= it->second.size)
            newSize = 0;
        else {
            newSize = int(it->second.size - it->second.offset);
        }
    }


    if (Core::Loader::getGameLoaderHandle()->getLoaderType() == Core::Loader::LOADER_TYPE_ISO) {
        auto loader = reinterpret_cast<Core::Loader::ISOLoader *>(Core::Loader::getGameLoaderHandle());

        if (it->second.info.logicalBlockAddress != -1) {
            loader->seek((uint64_t) it->second.info.logicalBlockAddress * 0x800 + it->second.offset, SEEK_SET);
            if (newSize != 0)
                loader->read(Core::Memory::getPointerUnchecked(data), newSize);
        } else {
            loader->seek(it->second.offset << 11, SEEK_SET);
            if (newSize != 0)
                loader->read(Core::Memory::getPointerUnchecked(data), (uint64_t)newSize << 11);
        }

        it->second.offset += newSize;
    } else {
        FILE *fp = fopen(it->second.hostDirectory.c_str(), "rb");

        fseek(fp, (long)it->second.offset, SEEK_SET);

        it->second.offset += newSize;
        if (newSize != 0) {
            fread(Core::Memory::getPointerUnchecked(data), newSize, 1, fp);
        }

        fclose(fp);
    }
    return newSize;
}

int sceIoRead(SceUID fd, uint32_t data, SceSize size) {
    int error;

    error = __hleIoRead(fd, data, size);
    if (error < 0) {
        LOG_ERROR(logType, "sceIoRead error 0x%08x addr 0x%08x", error, data);
        return error;
    }
    LOG_SYSCALL(logType, "sceIoRead(0x%06x, 0x%08x, 0x%08x) = %d", fd, data, size, error);
    return error;
}

int sceIoWrite(SceUID fd, uint32_t dataAddr, SceSize size) {
    std::string s;
    uint8_t *ptr = (uint8_t *)Memory::getPointerUnchecked(dataAddr);

    switch (fd) {
    case 2:
    case 1:
        if (size == 0)
            return 0;

        for (uint8_t *i = ptr; i < ptr + size; i++)
            s += *i;


        fwrite(s.c_str(), size, 1, p.get());

        if (s.c_str()[s.length() - 1] == '\n') {
            s = s.substr(0, s.length() - 1);
        }

        LOG_INFO(logType, "%d = console output: %s", size, s.c_str());
        return size;
    }

    LOG_WARN(logType, "unimplemented sceIoWrite(0x%08x, 0x%08x, 0x%08x)", fd, dataAddr, size);
    return size;
}

static int __ioClose(SceUID fd) {
    auto it = fileMap.find(fd);

    if (it == fileMap.end())
        return SCE_KERNEL_ERROR_BADF;

    fileMap.erase(fd);
    return 0;
}

int sceIoClose(SceUID fd) {
    int error = __ioClose(fd);

    if (error < 0)
        LOG_ERROR(logType, "can't close file descriptor 0x%06x", fd);
    else
        LOG_SYSCALL(logType, "sceIoClose(0x%06x)", fd);
    return error;
}

static SceOff __hleIoLseek(SceUID fd, SceOff offset, int whence) {
    auto it = fileMap.find(fd);
    if (it == fileMap.end()) {
        return (int32_t) SCE_KERNEL_ERROR_BADF;
    }

    if (whence < 0 || whence > 2) {
        return (int32_t) SCE_KERNEL_ERROR_INVAL;
    }

    switch (whence) {
    case SEEK_SET:
        it->second.offset = offset;
        break;
    case SEEK_CUR:
        it->second.offset += offset;
        break;
    case SEEK_END:
        it->second.offset = it->second.size;
        break;
    }
    return it->second.offset;
}

SceOff sceIoLseek(SceUID fd, SceOff offset, int whence) {
    SceOff off = __hleIoLseek(fd, offset, whence);

    if (off < 0) {
        LOG_ERROR(logType, "can't do sceIoLseek error code 0x%08x", offset);
        return off;
    }

    LOG_SYSCALL(logType, "sceIoLseek(0x%06x, 0x%08x, %d) = 0x%016llx", fd, offset, whence, off);
    return off;
}

uint32_t sceIoLseek32(SceUID fd, int offset, int whence) {
    int off = (int) __hleIoLseek(fd, offset, whence);

    if (off < 0) {
        LOG_ERROR(logType, "can't do sceIoLseek32 error code 0x%08x", offset);
        return off;
    }

    LOG_SYSCALL(logType, "sceIoLseek32(0x%06x, 0x%08x, %d) = 0x%08x", fd, offset, whence, off);
    return off;
}

#pragma pack(push, 1)
struct SceIoStat {
    SceMode st_mode;
    uint32_t st_attr;
    SceOff st_size; // Size of the file in bytes.
    ScePspDateTime sce_st_ctime; // Creation time.
    ScePspDateTime sce_st_atime; // Access time.
    ScePspDateTime sce_st_mtime; // Modification time.
    uint32_t st_private[6];
};
#pragma pack(pop)

int sceIoGetstat(const char *file, uint32_t statPtr) {
    SceIoStat *stat;

    if (!file) {
        LOG_ERROR(logType, "can't stat file while path is invalid");
        return SCE_KERNEL_ERROR_ERRNO_FILE_NOT_FOUND;
    }

    stat = (SceIoStat *)Core::Memory::getPointerUnchecked(statPtr);
    if (!stat) {
        LOG_ERROR(logType, "can't stat file if stat pointer is zero!");
        return -1;
    }

    if (auto loader = reinterpret_cast<Core::Loader::ISOLoader *>(Core::Loader::getGameLoaderHandle()); loader && loader->getLoaderType() == Core::Loader::LOADER_TYPE_ISO) {
        std::string path = removeBlockDevice(file);

        int lbn = 0, size = 0;

        if (path.starts_with("sce_lbn")) {
            (void)sscanf(path.c_str(), "sce_lbn0x%x_size0x%x", &lbn, &size);
            stat->st_mode = 0x21b6;
            stat->st_size = size;
            stat->st_private[0] = lbn;
        } else {
            Core::Loader::ISOFileInfo i;
            bool state = loader->getFileInfo(path, &i);

            if (!i.isDirectory) {
                stat->st_attr = 32;
                stat->st_mode = 0x21b6;
            } else {
                stat->st_attr = 16;
                stat->st_mode = 0x11ff;
            }

            stat->st_private[0] = i.logicalBlockAddress;
            stat->st_size = i.size;
        }
    } else {
        std::string hostFile = getHostDirectoryFromFile(file);

        FILE *f;
        int bufSize = 0;
        fopen_s(&f, hostFile.c_str(), "rb");

        if (!f) {
            LOG_ERROR(logType, "can't stat file while path is invalid (path %s) (host %s)", file, hostFile.c_str());
            return SCE_KERNEL_ERROR_ERRNO_FILE_NOT_FOUND;
        }

        fseek(f, 0, SEEK_END);
        bufSize = ftell(f);
        fclose(f);
        stat->st_size = bufSize;
        stat->st_mode = 0x21b6;
    }
    LOG_SYSCALL(logType, "sceIoGetstat(%s, 0x%08x)", file, statPtr);
    return 0;
}

int sceIoIoctl(SceUID fd, uint32_t cmd, uint32_t inAddr, int inlen, uint32_t outAddr, int outlen) {
    return 0;
}

static Core::Kernel::Callback *callback;

void setMemStickCallback(Core::Kernel::Callback *_callback) {
    callback = _callback;
}

Core::Kernel::Callback *getRegisteredMemStickCallback() {
    return callback;
}

int sceIoDevctl(const char *dev, uint32_t cmd, uint32_t inAddr, int inlen, uint32_t outAddr, int outlen) {
    switch (cmd) {
    case 0x02415821:
        if (auto ptr = (uint32_t *) Core::Memory::getPointer(inAddr); ptr && inlen >= 4) {
            auto cb = getKernelObject<Callback>(*ptr);
            setMemStickCallback(cb);
            cb->notifyArg = 1;
            hleCurrentThreadEnableCallbackState();
        }
        break;
    case 0x02425823:
        Core::Memory::write32(outAddr, 1);
        break;
    default:
        LOG_ERROR(logType, "unimplemented fatms0 cmd 0x%08x", cmd);
    }
    return 0;
}

#pragma pack(push, 1)
struct SceIoDirent {
    SceIoStat stat;
    char d_name[256];
    uint32_t d_private;
    int dummy;
};
#pragma pack(pop)

std::unordered_map<int, std::deque<SceIoDirent>> directoryFileMap;

SceUID sceIoDopen(const char *dirname) {
    if (!dirname) {
        LOG_ERROR(logType, "can't open directory from sceIoDopen!");
        return SCE_KERNEL_ERROR_ERRNO_FILE_NOT_FOUND;
    }

    std::deque<SceIoDirent> f;

    if (auto loader = reinterpret_cast<Core::Loader::ISOLoader *>(Core::Loader::getGameLoaderHandle()); loader && loader->getLoaderType() == Core::Loader::LOADER_TYPE_ISO) {
        std::string path = removeBlockDevice(dirname);

        if (path.c_str()[path.length() - 1] != '/')
            path += '/';

        Core::Loader::ISOFileInfo info;
        auto z = loader->getAllFiles();

        for (auto& i : z) {
            SceIoDirent d {};
            if (!strcmp(path.c_str(), i.path.c_str())) {
                std::memset(&d, 0xde, sizeof d.stat);

                std::strncpy(d.d_name, i.name.c_str(), i.name.length());
                d.d_name[255] = 0;
                if (!i.isDirectory) {
                    d.stat.st_attr = 32;
                    d.stat.st_mode = 0x21b6;
                } else {
                    d.stat.st_attr = 16;
                    d.stat.st_mode = 0x11ff;
                }

                d.stat.st_private[0] = i.logicalBlockAddress;
                d.stat.st_size = i.size;

                f.push_back(d);
            }
        }
       
        loader->getFileInfo(path, &info);

        int fd;
        std::unordered_map<int, std::deque<SceIoDirent>>::iterator p;
        do {
            fd = Core::Utility::getRandomInt();
            p = directoryFileMap.find(fd);
        } while (p != directoryFileMap.end());

        directoryFileMap[fd] = f;
        LOG_SYSCALL(logType, "sceIoDopen(%s)", dirname);
        return fd;
    }

    LOG_WARN(logType, "unimplemented sceIoDopen(%s)", dirname);
    return 0;
}

int sceIoDread(SceUID fd, uint32_t direntAddr) {
    auto it = directoryFileMap.find(fd);

    if (it == directoryFileMap.end()) {
        LOG_ERROR(logType, "can't read directory file descriptor 0x%06x!", fd);
        return SCE_KERNEL_ERROR_BADF;
    }

    if (it->second.size() != 0) {
        auto& dirent = it->second.at(0);
        it->second.pop_front();
        void *direntPtr = Core::Memory::getPointerUnchecked(direntAddr);
        if (direntPtr)
            std::memcpy(direntPtr, &dirent, sizeof dirent);

        LOG_SYSCALL(logType, "sceIoDread(0x%06x, 0x%08x)", fd, direntAddr);
        return 1;
    }

    if (it->second.size() == 0)
        return 0;

    LOG_SYSCALL(logType, "sceIoDread(0x%06x, 0x%08x)", fd, direntAddr);
    return -1;
}

int sceIoDclose(SceUID fd) {
    auto it = directoryFileMap.find(fd);

    if (it == directoryFileMap.end()) {
        LOG_ERROR(logType, "can't close directory file descriptor 0x%06x!", fd);
        return SCE_KERNEL_ERROR_BADF;
    }

    directoryFileMap.erase(fd);
    LOG_SYSCALL(logType, "sceIoDclose(0x%06x)", fd);
    return 0;
}

int sceIoChdir(const char *path) {
    cwd = path ? path : "";
    LOG_SYSCALL(logType, "sceIoChdir(path: %s)", path);
    return 0;
}

static uint64_t lastAsyncOperation = 0;

// async
SceUID sceIoOpenAsync(const char *file, int flags, SceMode mode) {
    SceUID uid = sceIoOpen(file, flags, mode);
    if (uid < 0)
        return SCE_KERNEL_ERROR_ERRNO_FILE_NOT_FOUND;

    lastAsyncOperation = (int32_t)uid;
    LOG_WARN(logType, "opened async file -^");
    return uid;
}

int sceIoReadAsync(SceUID fd, uint32_t data, SceSize size) {
    int error = __hleIoRead(fd, data, size);
    if (error < 0) {
        lastAsyncOperation = (int32_t)error;
        LOG_ERROR(logType, "can't do sceIoReadAsync error 0x%08x", error);
        return 0;
    }

    lastAsyncOperation = (int32_t)error;
    LOG_SYSCALL(logType, "sceIoReadAsync(0x%06x, 0x%08x, 0x%08x) = %d", fd, data, size, error);
    return 0;
}

int sceIoWriteAsync(SceUID fd, uint32_t dataAddr, SceSize size);
int sceIoCloseAsync(SceUID fd) {
    int error = __ioClose(fd);

    if (error < 0)
        LOG_ERROR(logType, "can't close async file descriptor 0x%06x", fd);
    else
        LOG_SYSCALL(logType, "sceIoClose(0x%06x)", fd);
    lastAsyncOperation = (int32_t)error;
    return error;
}

int sceIoLseekAsync(SceUID fd, SceOff offset, int whence) {
    SceOff off = __hleIoLseek(fd, offset, whence);

    if (off < 0) {
        LOG_ERROR(logType, "can't do sceIoLseekAsync error code 0x%08x", offset);
        return (int)off;
    }

    lastAsyncOperation = off;
    LOG_SYSCALL(logType, "sceIoLseekAsync(0x%06x, 0x%08x, %d) = 0x%016llx", fd, offset, whence, off);
    return 0;
}

int sceIoLseek32Async(SceUID fd, int offset, int whence) {
    SceOff off = (int)__hleIoLseek(fd, offset, whence);

    if (off < 0) {
        LOG_ERROR(logType, "can't do sceIoLseekAsync error code 0x%08x", offset);
        return (int)off;
    }

    lastAsyncOperation = off;
    LOG_SYSCALL(logType, "sceIoLseekAsync(0x%06x, 0x%08x, %d) = 0x%016llx", fd, offset, whence, off);
    return 0;
}

int sceIoIoctlAsync(SceUID fd, uint32_t cmd, uint32_t inAddr, int inlen, uint32_t outAddr, int outlen);

int sceIoWaitAsync(SceUID fd, uint32_t resultPtr);

static void __hlePollAsync(SceUID fd, uint32_t resultPtr) {
    Core::Memory::write32(resultPtr, (uint32_t)(lastAsyncOperation));
    Core::Memory::write32(resultPtr + 0x4, (uint32_t)(lastAsyncOperation >> 32uLL));
}

int sceIoWaitAsyncCB(SceUID fd, uint32_t resultPtr) {
    __hlePollAsync(fd, resultPtr);
    LOG_SYSCALL(logType, "sceIoWaitAsyncCB(0x%06x, 0x%08x)", fd, resultPtr);
    return 0;
}

int sceIoPollAsync(SceUID fd, uint32_t resultPtr) {
    __hlePollAsync(fd, resultPtr);
    LOG_SYSCALL(logType, "sceIoPollAsync(0x%06x, 0x%08x) = 0x%016llx", fd, resultPtr, lastAsyncOperation);
    return 0;
}
}