#include <Core/Kernel/Objects/Module.h>

#include <Core/Logger.h>

namespace Core::Kernel {
void moduleImportAddStub(ModuleImport *moduleImport, uint32_t nid, uint32_t address) {
    ModuleImport::Stub s;

    if (moduleImport->stubIndex < moduleImport->stubMaxSize) {
        s.nid = nid;
        s.address = address;
        moduleImport->stub[moduleImport->stubIndex++] = s;
    } else {
        LOG_ERROR("ModuleImport", "stub index >= %d", moduleImport->stubMaxSize);
        std::exit(0);
    }
}

void moduleImportReset(ModuleImport *moduleImport) {
    moduleImport->stubMaxSize = sizeof(moduleImport->stub) / sizeof(*moduleImport->stub);
    moduleImport->stubIndex = 0;
    std::memset(moduleImport->stub, 0x0, sizeof moduleImport->stub);
    std::memset(moduleImport->name, 0, sizeof moduleImport->name);
}

void moduleExportAddStub(ModuleExport *moduleExport, uint8_t type, uint32_t nid, uint32_t address) {
    ModuleExport::Entry e;
    if (moduleExport->entryIndex < moduleExport->entryMaxSize) {
        e.type = type;
        e.nid = nid;
        e.address = address;
        moduleExport->entry[moduleExport->entryIndex++] = e;
    } else {
        LOG_ERROR("ModuleExport", "entry index >= %d", moduleExport->entryMaxSize);
    }
}

void moduleExportReset(ModuleExport *moduleExport) {
    moduleExport->entryMaxSize = sizeof(moduleExport->entry) / sizeof(*moduleExport->entry);
    moduleExport->entryIndex = 0;
    std::memset(moduleExport->entry, 0x0, sizeof moduleExport->entry);
    std::memset(moduleExport->name, 0, sizeof moduleExport->name);
}

}