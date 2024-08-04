#pragma once

#include <string>
#include <cstdint>

#include <Core/PSP/Types.h>

namespace Core::Kernel { struct Callback; }

namespace Core::HLE {
void setHostCurrentWorkingDirectory(const std::string& path);

void setMemStickCallback(Core::Kernel::Callback *callback);
Core::Kernel::Callback *getRegisteredMemStickCallback();

SceUID sceIoOpen(const char *file, int flags, SceMode mode);
int sceIoRead(SceUID fd, uint32_t data, SceSize size);
int sceIoWrite(SceUID fd, uint32_t dataAddr, SceSize size);
int sceIoClose(SceUID fd);
SceOff sceIoLseek(SceUID fd, SceOff offset, int whence);
uint32_t sceIoLseek32(SceUID fd, int offset, int whence);
int sceIoGetstat(const char *file, uint32_t statPtr);
int sceIoIoctl(SceUID fd, uint32_t cmd, uint32_t inAddr, int inlen, uint32_t outAddr, int outlen);
int sceIoDevctl(const char *dev, uint32_t cmd, uint32_t inAddr, int inlen, uint32_t outAddr, int outlen);

SceUID sceIoDopen(const char *dirname);
int sceIoDread(SceUID fd, uint32_t direntAddr);
int sceIoDclose(SceUID fd);

int sceIoChdir(const char *path);

// async
SceUID sceIoOpenAsync(const char *file, int flags, SceMode mode);
int sceIoReadAsync(SceUID fd, uint32_t data, SceSize size);
int sceIoWriteAsync(SceUID fd, uint32_t dataAddr, SceSize size);
int sceIoCloseAsync(SceUID fd);
int sceIoLseekAsync(SceUID fd, SceOff offset, int whence);
int sceIoLseek32Async(SceUID fd, int offset, int whence);
int sceIoIoctlAsync(SceUID fd, uint32_t cmd, uint32_t inAddr, int inlen, uint32_t outAddr, int outlen);

int sceIoWaitAsync(SceUID fd, uint32_t resultPtr);
int sceIoWaitAsyncCB(SceUID fd, uint32_t resultPtr);
int sceIoPollAsync(SceUID fd, uint32_t resultPtr);
}