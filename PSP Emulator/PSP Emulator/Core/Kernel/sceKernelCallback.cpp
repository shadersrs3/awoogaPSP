#if defined (_MSC_VER) || defined(_WIN32) || defined (_WIN64)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <Core/Kernel/sceKernelCallback.h>
#include <Core/Kernel/Objects/Callback.h>
#include <Core/Kernel/sceKernelThread.h>

#include <Core/Logger.h>

namespace Core::Kernel {
static const char *logType = "sceKernelCallback";

int sceKernelCreateCallback(const char *name, uint32_t handler, uint32_t arg) {
    if (!name)
        return SCE_KERNEL_ERROR_ERROR;

    if ((handler & 0xF000'0000) != 0)
        return SCE_KERNEL_ERROR_ILLEGAL_ADDR;

    Callback *c = createKernelObject<Callback>();
    if (!c)
        return SCE_KERNEL_ERROR_OUT_OF_MEMORY;

    std::strncpy(c->name, name, 32);
    c->handler = handler;
    c->notifyCount = 0;
    c->notifyArg = 0;
    c->common = arg;
    c->ready = false;

    if (current)
        c->threadId = current->getUID();
    else
        c->threadId = 0;

    saveKernelObject(c);
    LOG_SYSCALL(logType, "sceKernelCreateCallback(%s, 0x%08x, 0x%08x) = 0x%06x", name, handler, arg, c->getUID());
    return c->getUID();
}

int sceKernelCancelCallback(SceUID cb) {

}

int sceKernelDeleteCallback(SceUID cb) {
    int error;

    destroyKernelObject(cb, &error);
    LOG_SYSCALL(logType, "sceKernelDeleteCallback(0x%06x) error 0x%08x", cb, error);
    return error;
}

int sceKernelNotifyCallback(SceUID cb, int arg2) {

}

void callbackInit() {

}

void callbackReset() {

}

void callbackDestroy() {

}
}