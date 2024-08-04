#pragma once

#include <string>

#include <Core/Kernel/Kernel.h>

namespace Core::Kernel::Debugger {
    void printKernelObject(KernelObject *object);
    void setKernelObjectParameter(KernelObject *object, const std::string& parameterKey, void *data, int size);
    void *getKernelObjectParameter(KernelObject *object, const std::string& parameterKey, int& size);
}