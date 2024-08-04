#if defined(_MSC_VER) || defined(_WIN32)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <cstring>

#include <Core/Kernel/Objects/Partition.h>

namespace Core::Kernel {
void Partition::setName(const char *name) {
    std::memset(this->name, 0, sizeof this->name);
    std::strncpy(this->name, name, sizeof this->name);
}
}