#include <Core/Loaders/ISOLoader.h>

#include <Core/Logger.h>

namespace Core::Loader {
static const char *logType = "isoLoader";
static constexpr int sectorStart = 0x10;
static constexpr int sectorSize = 0x800;
static constexpr int sectorStartOffset = sectorStart * sectorSize;

#pragma pack(push, 1)

/*
* Volume Descriptors
* File Descriptors
* Directory Descriptors
* Path Tables
*/

struct DirectoryRecord {
    uint8_t size;
    uint8_t sectorsExtendedRecord;
    uint32_t lbaLE;
    uint32_t lbaBE;
    uint32_t dataLengthLE;
    uint32_t dataLengthBE;
    uint8_t dateTimeEtc[6];
    uint8_t gmtOffset;
    uint8_t fileFlags;
    uint8_t fileUnitSize;
    uint8_t interleaveGapSize;
    uint16_t volumeSequenceNumberLE;
    uint16_t volumeSequenceNumberBE;
    uint8_t identifierLength;
    uint8_t data; // ...
};

struct RawVolumeDescriptor {
    uint8_t volumeDescriptorType; // type 0, 1, 2, 3, 255 are used
    uint8_t identifier[5];
    uint8_t volumeDescriptorVersion;
    uint8_t reserved[0x7F9];
};

struct PrimaryVolumeDescriptor {
    uint8_t volumeDescriptorType;
    uint8_t identifier[5];
    uint8_t version;
    uint8_t unused0;
    uint8_t systemIdentifier[32];
    uint8_t volumeIdentifier[32];
    uint8_t unused1[8];
    uint32_t volumeSpaceSizeLE;
    uint8_t volumeSpaceSizeBE[4];
    uint8_t unused2[32];
    uint16_t volumeSetSizeLE;
    uint8_t volumeSetSizeBE[2];
    uint16_t volumeSequenceNumberLE;
    uint8_t volumeSequenceNumberBE[2];
    uint16_t logicalBlockSizeLE;
    uint8_t logicalBlockSizeBE[2];
    uint32_t pathTableSizeLE;
    uint8_t pathTableSizeBE[4];
    uint32_t pathTableTypeLLocation;
    uint32_t pathTableTypeLLocationOptional;
    uint32_t pathTableTypeMLocation;
    uint32_t pathTableTypeMLocationOptional;
    DirectoryRecord directoryRecord; // level 1 directory (root)    
    uint8_t volumeSetIdentifier[128];
    uint8_t publisherIdentifier[128];
    uint8_t dataPreparerIdentifier[128];
    uint8_t applicationIdentifier[128];
    uint8_t copyrightFileIdentifier[37];
    uint8_t abstractFileIdentifier[37];
    uint8_t bibliographicFileIdentifier[37];
    uint8_t volumeCreationDateTime[17];
    uint8_t volumeModificationDateTime[17];
    uint8_t volumeExpirationDateTime[17];
    uint8_t volumeEffectiveDataTime[17];
    uint8_t fileStructureVersion;
    uint8_t unused3;
    uint8_t applicationData[512];
    uint8_t unused4[653];
};

struct PathTable {
    uint8_t directoryIdentifierLength;
    uint8_t extendedAttributeRecordLength;
    uint32_t lba;
    uint16_t parentDirectoryNumber;
    uint8_t data[256];
};

#pragma pack(pop)

ParamSFO *ISOLoader::getParamSFO() {
    return nullptr;
}

PNGData *ISOLoader::getPNG(int index) {
    return nullptr;
}

MPEGPlayer *ISOLoader::getMPEGPlayer() {
    return nullptr;
}

AT3Player *ISOLoader::getAT3Player() {
    return nullptr;
}

bool ISOLoader::getFileInfo(const std::string& path, ISOFileInfo *info) {
    for (auto& i : getAllFiles()) {
        if (i.path + i.name == path) {
            info->path = i.path;
            info->name = i.name;
            info->isDirectory = i.isDirectory;
            info->size = i.size;
            info->fileOffset = i.fileOffset;
            info->logicalBlockAddress = i.logicalBlockAddress;
            return true;
        }
    }
    return false;
}


std::vector<ISOFileInfo> ISOLoader::getAllFiles() {
    return isoFileInfo;
}

void ISOLoader::pushFileInfo(const ISOFileInfo& info) {
    isoFileInfo.push_back(info);
}

static void parseFiles(ISOLoader *loader, DirectoryRecord *rec, std::string path = "") {
    static int directoryLevel = 0;
    uint32_t offset = rec->lbaLE * sectorSize;

    bool pathSet = false;

    const char *name;
    uint8_t data[256];

    bool debug = false;

    while (true) {
        int oldOffset = offset;

        loader->seek(offset, SEEK_SET);
        loader->read(data, sizeof data);

        offset += data[0];

        if (!data[0]) {
            if ((offset & 0x7FF) > 0x700)
                offset = (offset + 0x7FF) & ~0x7FF;

            if (offset == oldOffset)
                break;

            loader->seek(offset, SEEK_SET);
            loader->read(data, sizeof data);

            if (!data[0]) {
                break;
            }
        }

        DirectoryRecord *d = reinterpret_cast<DirectoryRecord *>(data);

        if (d->size != 0x30) {
            ISOFileInfo info;
            name = reinterpret_cast<const char *>(&d->data);

            // LOG_DEBUG(logType, "ISO file %s%s", path.c_str(), name);

            info.path = path;
            info.name = name;
            info.fileOffset = d->lbaLE * sectorSize;
            info.logicalBlockAddress = d->lbaLE;
            info.size = d->dataLengthLE;

            if (d->fileFlags & 2) {
                // LOG_DEBUG(logType, "----- entering directory %s%s -----", path.c_str(), name);
                path += std::string(name) + "/";
                parseFiles(loader, d, path);
                path = path.substr(0, path.find_last_of('/'));
                path = path.substr(0, path.find_last_of('/') + 1);
                info.isDirectory = true;

                // LOG_DEBUG(logType, "----- exitting directory %s%s -----", path.c_str(), name);
            } else {
                info.isDirectory = false;
            }

            loader->pushFileInfo(info);
        }
    }
}

bool ISOLoader::parseISOFiles() {
    PrimaryVolumeDescriptor pvd;

    seek(sectorStartOffset, SEEK_SET);
    read(&pvd, sizeof pvd);

    if (pvd.logicalBlockSizeLE != 0x800) {
        LOG_ERROR(logType, "can't support ISO sector sizes other than 2048 bytes\n");
        return false;
    }

    if (pvd.volumeDescriptorType != 0x1) { // not a pvd
        LOG_ERROR(logType, "can't read ISO, primary volume descriptor is invalid\n");
        return false;
    }

    parseFiles(this, &pvd.directoryRecord);
    return true;
}

int ISOLoader::loadProgram() {
    auto _elfLoader = getELFLoader();

    if (!_elfLoader) {
        LOG_ERROR(logType, "can't get ELF loader context!");
        return -1;
    }
    return _elfLoader->loadProgram();
}
}