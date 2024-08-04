#pragma once

#include <Core/Loaders/AbstractLoader.h>
#include <Core/Loaders/ELFLoader.h>

namespace Core::Loader {
struct ISOFileInfo {
    std::string path;
    std::string name;
    bool isDirectory;
    size_t size;
    size_t fileOffset;
    int logicalBlockAddress;
};

struct ISOLoader : public AbstractLoader {
private:
    std::vector<ISOFileInfo> isoFileInfo;
public:
    ParamSFO *getParamSFO();
    PNGData *getPNG(int index);
    MPEGPlayer *getMPEGPlayer();
    AT3Player *getAT3Player();

    void pushFileInfo(const ISOFileInfo& info);
    bool getFileInfo(const std::string& path, ISOFileInfo *info);
    std::vector<ISOFileInfo> getAllFiles();

    bool parseISOFiles();
    int loadProgram() override;
};
}