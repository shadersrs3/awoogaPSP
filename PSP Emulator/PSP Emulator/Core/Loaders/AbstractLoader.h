#pragma once

#include <string>

namespace Core::Kernel { struct PSPModule; }

namespace Core::Loader {

enum LoaderType {
    LOADER_TYPE_UNKNOWN,
    LOADER_TYPE_ELF,
    LOADER_TYPE_ENCRYPTED_ELF,
    LOADER_TYPE_PBP,
    LOADER_TYPE_ISO,
    LOADER_TYPE_CSO,
};

struct FileHandle;

struct ELFState;

struct PNGData;
struct ParamSFO;
struct AT3Player;
struct MPEGPlayer;

struct ELFLoader;

struct AbstractLoader {
private:
    std::string executableFilename;
    LoaderType loaderType;
    FileHandle *handle;
    size_t fileSize;
    bool loadedProgram;
    ELFLoader *elfLoader;
    size_t elfFileOffset;
    size_t elfSize;
public:
    AbstractLoader();
    virtual ~AbstractLoader();

    FileHandle *getFileHandle();
    inline const LoaderType& getLoaderType() { return loaderType; }
    inline const size_t& getFileSize() { return fileSize; }

    inline void setFileHandle(FileHandle *handle) { this->handle = handle; }
    inline void setLoaderType(LoaderType type) { loaderType = type; }
    inline void setFileSize(size_t size) { fileSize = size; }
    inline void setLoadedProgram(bool state) { loadedProgram = state; }

    inline const bool& hasLoadedProgram() { return loadedProgram; }

    inline void setELFLoader(ELFLoader *loader) { elfLoader = loader; }
    inline ELFLoader *getELFLoader() { return elfLoader; }

    void setELFOffsetBase(size_t offset);
    size_t getELFOffsetBase();

    void seekELF(size_t offset, int seekType);

    void seek(size_t offset, int seekType);
    void read(void *ptr, size_t size);

    void setExecutableFilename(const std::string& filename) { executableFilename = filename; }
    const std::string& getExecutableFilename() { return executableFilename; }

    virtual int loadProgram() = 0;
};

AbstractLoader *load(const std::string& path);

Core::Kernel::PSPModule *getModule(AbstractLoader *loader);

void setGameLoaderHandle(AbstractLoader *handle);
AbstractLoader *getGameLoaderHandle();
}