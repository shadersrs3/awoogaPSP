#pragma once

#include <string>

#include <Core/Filesystem/PSPFilesystem.h>

namespace Core {

struct BlockDevice;

void blockDeviceInitContext();
void blockDeviceDestroyContext();

int blockDeviceCreate(const std::string& device);
BlockDevice *blockDeviceGetDevice(int handle);
BlockDevice *blockDeviceGetDevice(const std::string& device);
void blockDeviceDestroy(int handle);

bool blockDeviceAddFile(const FileInfo *info);
bool blockDeviceRemoveFile(const std::string& file);
void blockDeviceFlushAllDisk();
}