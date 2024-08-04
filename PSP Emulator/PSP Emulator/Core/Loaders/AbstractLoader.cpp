#if defined(_WIN32) || defined (_WIN64)
#   define _CRT_SECURE_NO_WARNINGS
#endif

#include <vector>

#include <Core/Loaders/AbstractLoader.h>

#include <Core/Loaders/ELFLoader.h>
#include <Core/Loaders/PBPLoader.h>
#include <Core/Loaders/ISOLoader.h>

#include <Core/Logger.h>

namespace Core::Loader {
static const char *logType = "loader";

AbstractLoader::AbstractLoader() :
    loaderType(LOADER_TYPE_UNKNOWN), handle(nullptr),
    fileSize(0), loadedProgram(false),
    elfLoader(nullptr), elfFileOffset(0), 
    elfSize(0) {}

AbstractLoader::~AbstractLoader() {
    if (handle) {
        fclose((FILE *)handle);
        handle = nullptr;
    }

    if (elfLoader) {
        if (!elfLoader->isBaseObject()) {
            delete elfLoader;
            elfLoader = nullptr;
        }
    }
}

static const char *getLoaderTypeName(LoaderType type) {
    switch (type) {
    case LOADER_TYPE_CSO: return "CSO";
    case LOADER_TYPE_ELF: return "ELF";
    case LOADER_TYPE_ENCRYPTED_ELF: return "Encrypted PRX";
    case LOADER_TYPE_ISO: return "ISO";
    case LOADER_TYPE_PBP: return "PBP";
    default: return "Unknown";
    }
}

FileHandle *AbstractLoader::getFileHandle() {
    return handle;
}

void AbstractLoader::setELFOffsetBase(size_t offset) {
    elfFileOffset = offset;
}

size_t AbstractLoader::getELFOffsetBase() {
    return elfFileOffset;
}

void AbstractLoader::seekELF(size_t offset, int seekType) {
    seek(getELFOffsetBase() + offset, seekType);
}

void AbstractLoader::seek(size_t offset, int seekType) {
    FILE *f = reinterpret_cast<FILE *>(handle);
    fseek(f, (long) offset, seekType);
}

void AbstractLoader::read(void *ptr, size_t size) {
    FILE *f = reinterpret_cast<FILE *>(handle);
    fread(ptr, size, 1, f);
}

static LoaderType getLoaderType(FileHandle *handle, size_t offset) {
    static uint8_t buf[16];
    LoaderType type;
    FILE *f = reinterpret_cast<FILE *>(handle);
    size_t oldOffset = ftell(f);

    std::memset(buf, 0, sizeof buf);

    fseek(f, (long) offset, SEEK_SET);
    fread(buf, 16, 1, f);
    if (!memcmp(buf, "\x7F\x45\x4C\x46", 4)) {
        type = LOADER_TYPE_ELF;
    } else if (!memcmp(buf, "~PSP", 4)) {
        type = LOADER_TYPE_ENCRYPTED_ELF;
    } else if (!memcmp(buf, "\x00PBP", 4)) {
        type = LOADER_TYPE_PBP;
    } else {
        fseek(f, 0x8000, SEEK_SET);
        fread(buf, 16, 1, f);
        if (!memcmp(buf, "\x01\x43\x44\x30\x30\x31\x01\x00", 8))
            type = LOADER_TYPE_ISO;
        else
            type = LOADER_TYPE_UNKNOWN;
    }

    fseek(f, (long) oldOffset, SEEK_SET);
    return type;
}

static size_t getLoaderFileSize(FileHandle *handle) {
    FILE *f = reinterpret_cast<FILE *>(handle);
    size_t size;

    if (!f)
        return 0;

    fseek(f, 0, SEEK_END);
    size = ftell(f);
    rewind(f);
    return size;
}

static FileHandle *openFile(const std::string& path) {
    FILE *f = fopen(path.c_str(), "rb");
    if (!f) {
        LOG_ERROR(logType, "can't open file \"%s\"", path.c_str());
        return nullptr;
    }

    return reinterpret_cast<FileHandle *>(f);
}

static void closeFile(FileHandle *handle) {
    fclose(reinterpret_cast<FILE *>(handle));
}

void setLoaderFilename(const std::string& path, AbstractLoader *loader) {
    const char n[2]{ '/', '\\' };
    for (int i = 0; i < 2; i++) {
        auto pos = path.find_last_of(n[i]);
        if (pos != std::string::npos) {
            loader->setExecutableFilename(path.substr(pos + 1, path.length() - pos));
            return;
        } else {
            loader->setExecutableFilename(path);
            return;
        }
    }
}

AbstractLoader *load(const std::string& path) {
    AbstractLoader *loader;
    FileHandle *handle;

    size_t fileSize;
    LoaderType type;

    handle = openFile(path);
    if (!handle)
        return nullptr;

    fileSize = getLoaderFileSize(handle);
    type = getLoaderType(handle, 0);

    if (type == LOADER_TYPE_UNKNOWN) {
        closeFile(handle);
        return nullptr;
    }

    LOG_INFO(logType, "file type %s, file size %ld KB, path: %s", getLoaderTypeName(type), fileSize >> 10, path.c_str());

    ELFLoader *elfLoader;

    switch (type) {
    case LOADER_TYPE_ISO:
    {
        ISOFileInfo info;
        loader = new ISOLoader;

        loader->setFileHandle(handle);
        loader->setFileSize(fileSize);
        loader->setLoaderType(type);

        if (!reinterpret_cast<ISOLoader *>(loader)->parseISOFiles()) {
            LOG_ERROR(logType, "can't parse iso files");
            delete loader;
            loader = nullptr;
            break;
        }

        bool valid = reinterpret_cast<ISOLoader *>(loader)->getFileInfo("PSP_GAME/SYSDIR/EBOOT.BIN", &info);
        if (!valid) {
            LOG_ERROR(logType, "can't find PSP_GAME/SYSDIR/EBOOT.BIN on ISO Filesystem!");
            delete loader;
            loader = nullptr;
            break;
        } else {
            loader->setExecutableFilename("PSP_GAME/SYSDIR/EBOOT.BIN");
        }

        LoaderType elfType;
        elfType = getLoaderType(handle, info.fileOffset);
        if (!(elfType == LOADER_TYPE_ELF || elfType == LOADER_TYPE_ENCRYPTED_ELF)) {
            LOG_ERROR(logType, "ISO Filesystem PSP_GAME/SYSDIR/EBOOT.BIN is not an ELF!");
            delete loader;
            loader = nullptr;
            break;
        }

        elfLoader = new ELFLoader(loader);
        elfLoader->setLoaderType(elfType);
        elfLoader->setBaseObject(false);
        elfLoader->setELFOffsetBase(info.fileOffset);
        elfLoader->setFileSize(info.size);

        loader->setELFLoader(elfLoader);
        loader->setELFOffsetBase(info.fileOffset);
    }
        break;
    case LOADER_TYPE_ELF:
    case LOADER_TYPE_ENCRYPTED_ELF:
    {
        loader = new ELFLoader;

        loader->setFileHandle(handle);
        loader->setFileSize(fileSize);
        loader->setLoaderType(type);
        setLoaderFilename(path, loader);
        reinterpret_cast<ELFLoader *>(loader)->setBaseObject(true);
        reinterpret_cast<ELFLoader *>(loader)->setLoaderPrivate(loader);
        loader->setELFOffsetBase(0);
        loader->setELFLoader(reinterpret_cast<ELFLoader *>(loader));

        break;
    }

    case LOADER_TYPE_PBP:
    {
        size_t fileOffset = 0;
        size_t elfSize = 0;
        loader = new PBPLoader;
        loader->setFileHandle(handle);
        loader->setFileSize(fileSize);
        loader->setLoaderType(type);

        PBPHeader pbpHdr;
        loader->read(&pbpHdr, sizeof pbpHdr);
        loader->seek(0, SEEK_SET);

        fileOffset = pbpHdr.offsets[PBP_EXECUTABLE_PSP];
        elfSize = loader->getFileSize() - pbpHdr.offsets[PBP_EXECUTABLE_PSP];
        LoaderType elfType;
        elfType = getLoaderType(handle, fileOffset);
        if (!(elfType == LOADER_TYPE_ELF || elfType == LOADER_TYPE_ENCRYPTED_ELF)) {
            LOG_ERROR(logType, "PBP executable is not an ELF!");
            delete loader;
            loader = nullptr;
            break;
        }

        setLoaderFilename(path, loader);
        elfLoader = new ELFLoader(loader);
        elfLoader->setLoaderType(elfType);
        elfLoader->setBaseObject(false);
        elfLoader->setELFOffsetBase(fileOffset);
        elfLoader->setFileSize(elfSize);

        loader->setELFLoader(elfLoader);
        loader->setELFOffsetBase(fileOffset);
        break;
    }
    default:
        loader = nullptr;
        LOG_ERROR(logType, "unknown loader type!");
    }
    return loader;
}

Core::Kernel::PSPModule *getModule(AbstractLoader *loader) {
    switch (loader->getLoaderType()) {
    case LOADER_TYPE_ELF:
        [[fallthrough]];
    case LOADER_TYPE_ENCRYPTED_ELF:
        return static_cast<ELFLoader *>(loader)->getPSPModule();
    case LOADER_TYPE_ISO:
        if (!static_cast<ISOLoader *>(loader)->getELFLoader())
            return nullptr;
        return static_cast<ISOLoader *>(loader)->getELFLoader()->getPSPModule();
    case LOADER_TYPE_PBP:
        if (!static_cast<PBPLoader *>(loader)->getELFLoader())
            return nullptr;
        return static_cast<PBPLoader *>(loader)->getELFLoader()->getPSPModule();
    }
    return nullptr;
}

static AbstractLoader *loader;

void setGameLoaderHandle(AbstractLoader *handle) {
    loader = handle;
}

AbstractLoader *getGameLoaderHandle() {
    return loader;
}
}