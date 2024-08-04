#pragma once

#include <cstdint>

#include <unordered_map>

namespace Core::HLE {
struct HLEFunction {
    char name[32];
    uint32_t nid;
    int syscallID;
    void (*func)();
};

struct HLEModule {
    char name[32];
    std::unordered_map<uint32_t, HLEFunction *> functions;
};

HLEFunction *getHLEFunction(const char *moduleName, uint32_t nid);

void __hleInit();
void __hleReset();
void __hleDestroy();
}