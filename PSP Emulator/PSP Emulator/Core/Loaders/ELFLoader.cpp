#if defined (_MSC_VER) || defined(_WIN32) || defined (_WIN64)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <format>

#include <Core/Loaders/ELFLoader.h>

#include <Core/Crypto/PRXDecrypter.h>

#include <Core/Kernel/sceKernelFactory.h>
#include <Core/Kernel/sceKernelError.h>
#include <Core/Kernel/sceKernelSystemMemory.h>


#include <Core/Memory/MemoryAccess.h>

#include <Core/Logger.h>

using namespace ELF;

namespace Core::Loader {
using namespace Core::Kernel;

static const char *logType = "elfloader";

struct ModuleInfo {
    uint16_t m_attr;
    uint16_t m_version;
    char m_name[28];
    uint32_t m_gp;
    uint32_t m_exports; // .lib.ent
    uint32_t m_exp_end;
    uint32_t m_imports; // .lib.stub
    uint32_t m_imp_end;
};

struct StubHeader {
    uint32_t s_modulename;
    uint16_t s_version;
    uint16_t s_flags;
    uint16_t s_size; // var count in upper 8bits?
    uint16_t s_imports;
    uint32_t s_nid;
    uint32_t s_text;
};

struct EntHeader {
    uint32_t modulename;
    uint16_t version;
    uint16_t attr;
    uint8_t size;
    uint8_t vcount;
    uint16_t fcount;
    uint32_t resident;
};

ELFLoader::ELFLoader() {
    PSPModule *pspModule;

    pspModule = createKernelObject<PSPModule>();
    saveKernelObject(pspModule);
    this->pspModule = pspModule->getUID();

    setBaseObject(false);
    setFileHandle(nullptr);
    setFileSize(0);
    setLoaderType(LOADER_TYPE_ELF);
    std::memset(segmentVAddr, 0, sizeof segmentVAddr);
    _loaderPrivate = nullptr;
    _elfPointer = nullptr;
}

ELFLoader::ELFLoader(AbstractLoader *loader) {
    Core::Kernel::PSPModule *pspModule;

    pspModule = createKernelObject<PSPModule>();
    saveKernelObject(pspModule);
    this->pspModule = pspModule->getUID();

    setBaseObject(false);
    setFileHandle(loader->getFileHandle());

    if (loader->getLoaderType() == LOADER_TYPE_ELF || loader->getLoaderType() == LOADER_TYPE_ENCRYPTED_ELF) {
        setFileSize(loader->getFileSize());
        setLoaderType(loader->getLoaderType());
    } else {
        setFileSize(0);
        setLoaderType(LOADER_TYPE_ELF);
    }

    std::memset(segmentVAddr, 0, sizeof segmentVAddr);
    _loaderPrivate = loader;
    _elfPointer = nullptr;
}

void ELFLoader::loadProgramHeaders() {
    int programHeadersNum = ehdr.e_phnum;
    Elf32_Phdr *phdrStart, *phdrEnd;

    for (phdrStart = reinterpret_cast<Elf32_Phdr *>(getPointer(ehdr.e_phoff)), phdrEnd = phdrStart + programHeadersNum;
        phdrStart < phdrEnd; phdrStart++) {
        programHeaders.push_back(*phdrStart);
    }
}

void ELFLoader::loadSectionHeaders() {
    int sectionHeadersNum = ehdr.e_shnum;
    Elf32_Shdr *shdrStart, *shdrEnd;

    const char *sectionString = reinterpret_cast<const char *>(
        getPointer(reinterpret_cast<Elf32_Shdr *>(getPointer(ehdr.e_shoff + ehdr.e_shentsize * ehdr.e_shstrndx))->sh_offset));

    for (shdrStart = reinterpret_cast<Elf32_Shdr *>(getPointer(ehdr.e_shoff)), shdrEnd = shdrStart + sectionHeadersNum;
        shdrStart < shdrEnd; shdrStart++) {
        SectionHeader sectionHeader;

        const char *str = sectionString + shdrStart->sh_name;

        sectionHeader.name = str;
        sectionHeader.sectionType = shdrStart->sh_type;
        sectionHeader.sectionFlags = shdrStart->sh_flags;
        sectionHeader.sectionAddress = shdrStart->sh_addr;
        sectionHeader.sectionOffset = shdrStart->sh_offset;
        sectionHeader.sectionSize = shdrStart->sh_size;
        sectionHeader.sectionLink = shdrStart->sh_link;
        sectionHeader.sectionInfo = shdrStart->sh_info;
        sectionHeader.sectionAddressAlign = shdrStart->sh_addralign;
        sectionHeader.sectionEntrySize = shdrStart->sh_entsize;
        sectionHeaders.push_back(sectionHeader);
    }
}

void ELFLoader::loadExecutableHeader() {
    std::memcpy(&ehdr, _elfPointer, sizeof ehdr);
}

void ELFLoader::loadSymbols(uint32_t relocatedAddress) {
    auto symTabSection = getSectionHeader(".symtab");

    if (symTabSection != nullptr) {
        int symTabLink = symTabSection->sectionLink;

        auto strTabSection = getSectionHeader(symTabLink);
        const char *stringTable;

        Elf32_Sym *sym = reinterpret_cast<Elf32_Sym *>(getPointer(symTabSection->sectionOffset));

        int nrSyms = symTabSection->sectionSize / sizeof Elf32_Sym;

        if (!strTabSection) {
            stringTable = nullptr;
            LOG_WARN(logType, "can't find ELF string table");
        } else {
            stringTable = reinterpret_cast<const char *>(getPointer(strTabSection->sectionOffset));
        }

        for (int i = 0; i < nrSyms; i++) {
            int size = sym[i].st_size;

            if (size == 0)
                continue;

            std::string name;
            int type = sym[i].st_info & 0xF;
            int sectionIndex = sym[i].st_shndx;
            int value = sym[i].st_value;

            if (relocatedAddress != 0)
                value += relocatedAddress + sectionHeaders[sectionIndex].sectionAddress;

            if (stringTable) {
                name += stringTable + sym[i].st_name;
            } else {
                name = std::format("unk_0x{:08X}_size_0x{:08X}", value, size);
            }

            switch (type) {
            case STT_FUNC:
                // printf("%s %x %x\n", stringTable + sym[i].st_name, value, size);
                break;
            case STT_OBJECT:
                // printf("%s %x %x\n", stringTable + sym[i].st_name, value, size);
                break;
            }
        }
    }
}

SectionHeader *ELFLoader::getSectionHeader(const std::string& name, int *index) {
    int x = 0;

    for (auto& i : sectionHeaders) {
        if (i.name == name) {
            if (index)
                *index = x;
            return &i;
        }

        x++;
    }
    return nullptr;
}

SectionHeader *ELFLoader::getSectionHeader(int index) {
    if (index < 0 || index >= sectionHeaders.size())
        return nullptr;

    return &sectionHeaders[index];
}

void ELFLoader::resetELFHeaders() {
    programHeaders.clear();
    sectionHeaders.clear();
    symbolHeaders.clear();
}

uint8_t *ELFLoader::decryptAndAllocatePRX() {
    _loaderPrivate->seekELF(0, SEEK_SET);
    uint8_t *_elfPointer = new uint8_t[getFileSize()];
    _loaderPrivate->read(_elfPointer, getFileSize());
    Core::Crypto::decrypt(_elfPointer, _elfPointer, (uint32_t) getFileSize(), nullptr);
    _loaderPrivate->seekELF(0, SEEK_SET);
    return _elfPointer;
}

uint8_t *ELFLoader::elfAllocateFull() {
    _loaderPrivate->seekELF(0, SEEK_SET);
    uint8_t *_elfPointer = new uint8_t[getFileSize()];
    _loaderPrivate->read(_elfPointer, getFileSize());
    return _elfPointer;
}

uint8_t *ELFLoader::getPointer(size_t offset) {
    if (!_elfPointer)
        return nullptr;

    return _elfPointer + offset;
}

static void debugModuleInfo(const ModuleInfo *info) {
    LOG_DEBUG(logType, "Module Name: %s Attr: 0x%04x Version: 0x%04x", info->m_name, info->m_attr, info->m_version);
}

int ELFLoader::loadProgram() {
    int error = SCE_KERNEL_ERROR_OK;
    if (pspModule < 0)
        return -1;

    if (!_loaderPrivate) {
        LOG_ERROR(logType, "abstract loader is NULL and file size is zero!");
        return -1;
    }

    if (getFileSize() == 0) {
        LOG_ERROR(logType, "file size is zero!");
        return -1;
    }

    if (getFileHandle() == nullptr) {
        LOG_ERROR(logType, "loader file handle is NULL");
        return -1;
    }

    auto _kernelPSPModule = getPSPModule();
    if (!_kernelPSPModule) {
        LOG_ERROR(logType, "PSP module is NULL");
        return -1;
    }

    uint8_t magic[4];
    _loaderPrivate->seekELF(0, SEEK_SET);
    _loaderPrivate->read(magic, sizeof magic);

    if (!memcmp(magic, "\x7F\x45\x4C\x46", 4)) {
        _elfPointer = elfAllocateFull();
        if (!_elfPointer)
            return -1;
    } else if (!memcmp(magic, "~PSP", 4)) {
        _elfPointer = decryptAndAllocatePRX();

        if (!_elfPointer)
            return -1;
    } else {
        LOG_ERROR(logType, "unknown ELF!");
        return -1;
    }

    resetELFHeaders();
    loadExecutableHeader();
    loadProgramHeaders();
    loadSectionHeaders();

    bool shouldRelocate = ehdr.e_type != ET_EXEC;
    uint32_t __rebaseAddress;

    auto rebaseAddress = [&__rebaseAddress](uint32_t address, bool shouldRelocate = false) {
        if (shouldRelocate)
            return address + __rebaseAddress;
        return address;
    };

    uint32_t totalStart = 0x7FFFFFFF;
    uint32_t totalEnd = 0x00000000;
    uint32_t totalSize;

    for (int i = 0; i < programHeaders.size(); i++) {
        const auto& phdr = programHeaders[i];

        if (phdr.p_type == PT_LOAD) {
            if (phdr.p_vaddr < totalStart)
                totalStart = phdr.p_vaddr;

            if (phdr.p_vaddr + phdr.p_memsz > totalEnd)
                totalEnd = phdr.p_vaddr + phdr.p_memsz;
        
            LOG_DEBUG(logType, "program header vaddr 0x%08x file size 0x%08x", phdr.p_vaddr, phdr.p_filesz);
        }
    }

    totalSize = totalEnd - totalStart;

    MemoryAllocator *m;

    m = Core::Kernel::getMemoryAllocatorFromID(2);

    if (shouldRelocate) {
        __rebaseAddress = m->allocLow(totalSize, "relocatable program");
    } else {
        __rebaseAddress = m->allocAt(totalStart, totalSize, "executable program");
    }

    totalStart = rebaseAddress(totalStart, shouldRelocate);
    totalEnd = rebaseAddress(totalEnd, shouldRelocate);

    LOG_DEBUG(logType, "total memory size 0x%08x address start 0x%08x, address end 0x%08x relocatable: %s", totalSize, totalStart, totalEnd, shouldRelocate ? "yes" : "no");

    int segmentAddressIndex = 0;
    std::memset(_kernelPSPModule->segmentAddr, 0, sizeof _kernelPSPModule->segmentAddr);
    std::memset(_kernelPSPModule->segmentSize, 0, sizeof _kernelPSPModule->segmentSize);
    _kernelPSPModule->numSegments = 0;
    for (int i = 0; i < programHeaders.size(); i++) {
        const auto& phdr = programHeaders[i];
        if (phdr.p_type == PT_LOAD) {
            segmentVAddr[i] = rebaseAddress(phdr.p_vaddr, shouldRelocate);
            uint32_t addressPointer = segmentVAddr[i];
            
            uint32_t fileSize = phdr.p_filesz;
            uint32_t memorySize = phdr.p_memsz;

            uint8_t *data = getPointer(phdr.p_offset);
            size_t programFileSize = phdr.p_offset + phdr.p_filesz;

            if (programFileSize > getFileSize()) {
                // Truncate
                LOG_WARN(logType, "truncating ELF size from 0x%08x to 0x%08x", programFileSize, getFileSize());
                programFileSize = getFileSize();
            }

            if (Memory::Utility::isValidAddressRange(addressPointer, addressPointer + fileSize)) {
                int bssSize = memorySize - fileSize;
                if (bssSize > 0)
                    Memory::Utility::memorySet(addressPointer + fileSize, 0x00, bssSize);

                LOG_DEBUG(logType, "copied program memory index %d file size 0x%08x", i, fileSize);;
                Memory::Utility::copyMemoryHostToGuest(addressPointer, data, fileSize);
            }
        
            _kernelPSPModule->segmentAddr[segmentAddressIndex] = segmentVAddr[i];
            _kernelPSPModule->segmentSize[segmentAddressIndex] = memorySize;
            _kernelPSPModule->numSegments++;
            ++segmentAddressIndex;
        }
    }

    // Module info handling
    {
        do {
            auto shdr = getSectionHeader(".rodata.sceModuleInfo");
            if (!shdr) {
                LOG_ERROR(logType, "no sceModuleInfo section found!");
                error = -1;
                break;
            }

            auto moduleInfo = (ModuleInfo *) getPointer(shdr->sectionOffset);
            std::memset(_kernelPSPModule->moduleName, 0, sizeof _kernelPSPModule->moduleName);
            std::strncpy(_kernelPSPModule->moduleName, moduleInfo->m_name, sizeof _kernelPSPModule->moduleName - 1);
            _kernelPSPModule->entryAddress = rebaseAddress(ehdr.e_entry, shouldRelocate);

            std::memset(_kernelPSPModule->fileNameForStack, 0, sizeof _kernelPSPModule->fileNameForStack);
            std::strncpy(_kernelPSPModule->fileNameForStack, _loaderPrivate->getExecutableFilename().c_str(), sizeof _kernelPSPModule->fileNameForStack - 1);

            _kernelPSPModule->isFromUMD = _loaderPrivate->getLoaderType() == LOADER_TYPE_ISO;

            _kernelPSPModule->stubTop = rebaseAddress(moduleInfo->m_imports, shouldRelocate);
            _kernelPSPModule->stubSize = moduleInfo->m_imp_end - moduleInfo->m_imports;

            _kernelPSPModule->entTop = rebaseAddress(moduleInfo->m_exports, shouldRelocate);
            _kernelPSPModule->entSize = moduleInfo->m_exp_end - moduleInfo->m_exports;

            _kernelPSPModule->gp = moduleInfo->m_gp >= 0x8000000 ? moduleInfo->m_gp : moduleInfo->m_gp + __rebaseAddress;
#if 0
            if (moduleInfo->m_gp >= 0x8000000) {
                _kernelPSPModule->gp = moduleInfo->m_gp;
            } else {
                if (moduleInfo->m_gp == 0)
                    _kernelPSPModule->gp = 0;
                else
                    _kernelPSPModule->gp = moduleInfo->m_gp + __rebaseAddress;
            }
#endif
            _kernelPSPModule->moduleAttribute = moduleInfo->m_attr;
            _kernelPSPModule->version[0] = moduleInfo->m_version & 0xFF;
            _kernelPSPModule->version[1] = (moduleInfo->m_version >> 8) & 0xFF;
            
            int stubLen = _kernelPSPModule->stubSize / sizeof(StubHeader);
            int entLen = _kernelPSPModule->entSize / sizeof(EntHeader);

            LOG_INFO(logType, "module name: \"%s\", entry address: 0x%08X", _kernelPSPModule->moduleName, _kernelPSPModule->entryAddress);

            for (int i = 0; i < stubLen; i++) {
                uint32_t stubAddress = _kernelPSPModule->stubTop + i * sizeof(StubHeader);
                auto stubHeader = (StubHeader *)Memory::getPointer(stubAddress);

                if (!stubHeader) {
                    LOG_ERROR(logType, "can't find stub header address 0x%08x", stubAddress);
                    error = -1;
                    break;
                }


                const char *importModuleName = (const char *) Memory::getPointer(rebaseAddress(stubHeader->s_modulename, shouldRelocate));

                if (!importModuleName) {
                    LOG_ERROR(logType, "unknown module import name address");
                    error = -1;
                    break;
                }

                uint32_t nidAddress = rebaseAddress(stubHeader->s_nid, shouldRelocate);
                uint32_t textAddress = rebaseAddress(stubHeader->s_text, shouldRelocate);

                ModuleImport modimp;
                
                Core::Kernel::moduleImportReset(&modimp);
                std::strncpy(modimp.name, importModuleName, sizeof modimp.name - 1);
                modimp.name[sizeof modimp.name - 1] = 0;

                for (int imp = 0; imp < stubHeader->s_imports; imp++) {
                    uint32_t importNID = nidAddress + imp * 4;
                    uint32_t hookAddress = textAddress + imp * 8;

                    if (!Memory::Utility::isValidAddress(importNID)) {
                        LOG_ERROR(logType, "invalid NID address 0x%08x name %s", importNID, importModuleName);
                        break;
                    }

                    uint32_t nid = Memory::read32(importNID);
                    Core::Kernel::moduleImportAddStub(&modimp, nid, hookAddress);
                    // LOG_DEBUG(logType, "%s: imported NID 0x%08X syscall address 0x%08X", importModuleName, nid, hookAddress);
                }

                _kernelPSPModule->moduleImport.push_back(modimp);
            }

            int dataSize = 0;
            int textSize = 0;
            int bssSize = 0;

            for (auto& sectionHeader : sectionHeaders) {
                uint32_t flags = sectionHeader.sectionFlags;
                if ((flags & SHF_ALLOC) && (flags & SHF_WRITE) && !(flags & SHF_MASKPROC)) {
                    dataSize += sectionHeader.sectionSize;
                }
                
                if ((flags & SHF_ALLOC) && !(flags & SHF_WRITE) && !(flags & SHF_STRINGS)) {
                    textSize += sectionHeader.sectionSize;
                }

                if (sectionHeader.name == ".bss") {
                    bssSize += sectionHeader.sectionSize;
                    dataSize -= bssSize;
                }
            }

            // _kernelPSPModule->textAddr = getSectionHeader(".text")->sectionAddress;
            _kernelPSPModule->textAddr = 0;
            _kernelPSPModule->textSize = textSize;
            _kernelPSPModule->dataSize = dataSize;
            _kernelPSPModule->bssSize = bssSize;
        } while (0);
    }

    // Now relocate
    for (int i = 0; i < programHeaders.size(); i++) {
        auto& programHeader = programHeaders[i];

        if (programHeader.p_type == PT_PSPREL1) {
            LOG_ERROR(logType, "can't relocate PRX program yet!");
            error = -1;
        } else if (programHeader.p_type == PT_PSPREL2) {
            LOG_ERROR(logType, "can't relocate PRX program yet!");
            error = -1;
        }
    }

    // Relocate from the section headers
    {
        auto relocateSection = [&](SectionHeader& sectionHeader) -> bool {
            std::vector<uint32_t> deferredHi16;
            int AHL;

            if (!(sectionHeaders[sectionHeader.sectionInfo].sectionFlags & SHF_ALLOC))
                return true;

            int numRelocs = sectionHeader.sectionSize / sizeof(Elf32_Rel);
            Elf32_Rel *rels = (Elf32_Rel *) getPointer(sectionHeader.sectionOffset);

            LOG_DEBUG(logType, "Number of section \"%s\" relocations %d", sectionHeader.name.c_str(), numRelocs);

            AHL = 0;
            for (int r = 0; r < numRelocs; r++) {
                uint32_t info = rels[r].r_info;
                uint32_t relocationOffset = rels[r].r_offset;

                uint32_t relocationType = info & 0xFF;
                uint32_t addressInfoOffset = (info >> 8) & 0xFF;
                uint32_t addressInfoBase = (info >> 16) & 0xFF;

                uint32_t addressOffsetAddress = segmentVAddr[addressInfoOffset];
                uint32_t addressBaseAddress = segmentVAddr[addressInfoBase];

                uint32_t relocationAddress = addressOffsetAddress + relocationOffset;

                int A = 0;
                int S = addressBaseAddress;

                uint32_t relocationData = Memory::read32(relocationAddress);
                uint32_t rMipsWord32 = relocationData & 0xFFFFFFFF; // <=> data;
                uint32_t rMipsTarget26 = relocationData & 0x03FFFFFF;
                uint32_t rMipsHi16 = relocationData & 0xFFFF;
                uint32_t rMipsLo16 = relocationData & 0xFFFF;
                uint32_t result;
                uint32_t *p;
                switch (relocationType) {
                case R_MIPS_NONE:
                    break;
                case R_MIPS_26:
                {
                    A = rMipsTarget26;
                    result = ((A << 2) + S) >> 2;
                    relocationData &= ~0x03FFFFFF;
                    relocationData |= result;
                }
                break;
                case R_MIPS_32:
                    relocationData = relocationData + S;
                    break;
                case R_MIPS_HI16:
                {
                    bool foundAddress = false;
                    A = rMipsHi16;
                    AHL = rMipsHi16 << 16;
                    deferredHi16.push_back(relocationAddress);
                    break;
                }
                case R_MIPS_LO16:
                    A = rMipsLo16;
                    AHL = (AHL & ~0x0000FFFF) | (A & 0xFFFF);
                    result = AHL + S;
                    relocationData &= ~0xFFFF;
                    relocationData |= result & 0xFFFF;

                    for (auto i = deferredHi16.begin(); i != deferredHi16.end(); ) {
                        uint32_t relocationData2 = Memory::read32(*i);

                        result = ((relocationData2 & 0xFFFF) << 16) + A + S;
                        if ((A & 0x8000) != 0) {
                            result -= 0x10000;
                        }
                        if ((result & 0x8000) != 0) {
                            result += 0x10000;
                        }

                        relocationData2 &= ~0xFFFF;
                        relocationData2 |= ((result >> 16) & 0xFFFF);

                        p = (uint32_t *) Memory::getPointerUnchecked(*i);
                        if (p)
                            *p = relocationData2;
                        i = deferredHi16.erase(i);
                    }
                    break;
                case R_MIPS_16:
                    LOG_ERROR(logType, "Unimplemented R_MIPS_16!");
                    error = -1;
                    continue;
                case 7:
                    LOG_WARN(logType, "Unknown GP relocation!");
                    // error = -1;
                    continue;
                default:
                    LOG_ERROR(logType, "Unimplemented relocation type %d\n", relocationType);
                    error = -1;
                    continue;
                }

                
                p = (uint32_t *) Memory::getPointerUnchecked(relocationAddress);
                if (p)
                    *p = relocationData;
            }
            return true;
        };

        for (int i = 0; i < sectionHeaders.size(); i++) {
            auto& sectionHeader = sectionHeaders[i];

            if (sectionHeader.sectionType == SHT_PSPREL) {
                if (!relocateSection(sectionHeader)) {
                    error = -1;
                    break;
                }
            }
        }

    }

    loadSymbols(shouldRelocate ? __rebaseAddress : 0);

    delete _elfPointer;
    _elfPointer = nullptr;
    if (error != SCE_KERNEL_ERROR_OK)
        LOG_SUCCESS(logType, "successfully placed ELF to memory");
    return error;
}

void ELFLoader::setLoaderPrivate(AbstractLoader *loaderPrivate) {
    _loaderPrivate = loaderPrivate;
}

PSPModule *ELFLoader::getPSPModule() {
    return getKernelObject<PSPModule>(pspModule);
}
}