#pragma once

#include <cstdint>

namespace Core::GPU {
int sceGeListEnQueue(uint32_t listAddress, uint32_t stallAddress, int cbid, uint32_t argPtr);
int sceGeListDeQueue(int qid);
int sceGeListUpdateStallAddr(int qid, uint32_t stallAddress);
int sceGeEdramGetSize();
uint32_t sceGeEdramGetAddr();
int sceGeSetCallback(uint32_t cbPtr);
int sceGeUnsetCallback(int cbid);
int sceGeDrawSync(int syncType);
int sceGeListSync(int qid, int syncType);
}