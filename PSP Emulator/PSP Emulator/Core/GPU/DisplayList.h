#pragma once

#include <cstdint>

namespace Core::GPU {
struct DisplayList {
    uint32_t id;
    uint32_t initialAddress;
    uint32_t stallAddress;
    uint32_t currentAddress;
    int callbackId;
    uint32_t callbackArgument;
    uint32_t callStack[8];
    int callStackCurrentIndex;
    uint32_t state;
};

DisplayList *getDisplayListFromQueue(int qid);
bool addDisplayListToQueue(const DisplayList& list);
DisplayList *getDisplayListFromQueue();
bool displayListInStallAddress(const DisplayList *dl);
void displayListRun(DisplayList *dl, int steps = 10000000);
}