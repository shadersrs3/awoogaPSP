#pragma once

#include <vector>

#include <Core/Loaders/AbstractLoader.h>

#include <Core/Loaders/ELFHeader.h>

#include <Core/Kernel/Objects/Module.h>

namespace Core::Loader {
struct SectionHeader {
    std::string name;
    uint32_t sectionType;
    uint32_t sectionFlags;
    uint32_t sectionAddress;
    uint32_t sectionOffset;
    uint32_t sectionSize;
    uint32_t sectionLink;
    uint32_t sectionInfo;
    uint32_t sectionAddressAlign;
    uint32_t sectionEntrySize;
};

struct ELFLoader : public AbstractLoader {
private:
    SceUID pspModule;
    bool baseObject;

    uint32_t segmentVAddr[256];
    ELF::Elf32_Ehdr ehdr;
    std::vector<ELF::Elf32_Phdr> programHeaders;
    std::vector<SectionHeader> sectionHeaders;
    std::vector<ELF::Elf32_Sym> symbolHeaders;
    uint8_t *_elfPointer;
    AbstractLoader *_loaderPrivate;
private:
    void loadProgramHeaders();
    void loadSectionHeaders();

    void loadExecutableHeader();
    void loadSymbols(uint32_t relocatedAddress);

    void relocate(int nrRelocs);
    void relocate2(int nrRelocs);

    SectionHeader *getSectionHeader(const std::string& name, int *index = nullptr);
    SectionHeader *getSectionHeader(int index);
    void resetELFHeaders();
    uint8_t *decryptAndAllocatePRX();
    uint8_t *elfAllocateFull();
    uint8_t *getPointer(size_t offset);
public:
    ELFLoader();
    ELFLoader(AbstractLoader *loader);

    int loadProgram() override;

    void setLoaderPrivate(AbstractLoader *loaderPrivate);

    inline void setBaseObject(bool state) { baseObject = state; }
    inline bool isBaseObject() { return baseObject; }

    Core::Kernel::PSPModule *getPSPModule();
};
}