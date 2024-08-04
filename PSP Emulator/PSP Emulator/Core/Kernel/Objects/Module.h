#pragma once

#include <Core/Kernel/Objects/KernelObject.h>

namespace Core::Kernel {
struct ModuleImport {
    struct Stub { uint32_t nid, address; };

    char name[32];
    int stubIndex;
    int stubMaxSize;
    Stub stub[0x200];
};

struct ModuleExport {
    struct Entry { uint8_t type; uint32_t nid; uint32_t address; };
    char name[32];
    int entryIndex;
    int entryMaxSize;
    Entry entry[0x200];
};

void moduleImportAddStub(ModuleImport *moduleImport, uint32_t nid, uint32_t address);
void moduleImportReset(ModuleImport *moduleImport);

void moduleExportAddEntry(ModuleExport *moduleExport, uint8_t type, uint32_t nid, uint32_t address);
void moduleExportReset(ModuleExport *moduleExport);

struct PSPModule : public KernelObject {
private:
    static constexpr KernelObjectType staticTypeId = KERNEL_OBJECT_MODULE;
public:
    char moduleName[28];
    char fileNameForStack[128];
    bool isFromUMD;
    uint16_t moduleAttribute;
    uint32_t entryAddress;
    uint32_t gp;

    uint32_t entTop;
    uint32_t entSize;
    uint32_t stubTop;
    uint32_t stubSize;

    uint32_t textAddr;
    uint32_t textSize;
    uint32_t dataSize;
    uint32_t bssSize;
    unsigned char version[2];
    int numSegments;
    int segmentAddr[4];
    int segmentSize[4];
    std::vector<ModuleImport> moduleImport;
    std::vector<ModuleExport> moduleExport;
public:
    constexpr static KernelObjectType getStaticTypeId() { return staticTypeId; }
    static constexpr const char *getStaticTypeName() { return "PSP Module"; }
    const char *getTypeName() override { return "PSP Module"; }
};
}