#pragma once

#include <Core/Kernel/Objects/EventFlag.h>

namespace Core::Kernel {
/** Event flag wait types */
enum {
    /** Wait for all bits in the pattern to be set */
    PSP_EVENT_WAITAND = 0x00,
    /** Wait for one or more bits in the pattern to be set */
    PSP_EVENT_WAITOR = 0x01,
    /** Clear the entire pattern when it matches. */
    PSP_EVENT_WAITCLEARALL = 0x10,
    /** Clear the wait pattern when it matches */
    PSP_EVENT_WAITCLEAR = 0x20,

    PSP_EVENT_WAITKNOWN = PSP_EVENT_WAITCLEAR | PSP_EVENT_WAITCLEARALL | PSP_EVENT_WAITOR,
};

SceUID sceKernelCreateEventFlag(const char *name, SceUInt attr, int bits, uint32_t optionPtr);
int sceKernelDeleteEventFlag(int evid);
int sceKernelClearEventFlag(SceUID evid, uint32_t bits);
int sceKernelPollEventFlag(SceUID evid, uint32_t bits, uint32_t wait, uint32_t outBitsPtr);
int sceKernelReferEventFlagStatus(SceUID evid, uint32_t statusPtr);
int sceKernelSetEventFlag(SceUID evid, uint32_t bits);
int sceKernelWaitEventFlag(SceUID evid, uint32_t bits, uint32_t wait, uint32_t outBitsPtr, uint32_t timeoutPtr);
int sceKernelWaitEventFlagCB(SceUID evid, uint32_t bits, uint32_t wait, uint32_t outBitsPtr, uint32_t timeoutPtr);
}