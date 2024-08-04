#pragma once

#include <Core/PSP/Types.h>

namespace Core::Kernel {
// forward declarations
struct KernelObject;
struct Alarm;
struct PSPThread;
struct EventFlag;
struct FPL;
struct VPL;
struct LwMutex;
struct Mutex;
struct MessageBox;
struct Semaphore;
struct PSPModule;
struct Partition;
struct Callback;

template KernelObject *createKernelObject(int *);
template Alarm *createKernelObject(int *);
template PSPThread *createKernelObject(int *);
template EventFlag *createKernelObject(int *);
template FPL *createKernelObject(int *);
template VPL *createKernelObject(int *);

template KernelObject *getKernelObject(SceUID, int *);
template Alarm *getKernelObject(SceUID, int *);
template PSPThread *getKernelObject(SceUID, int *);
template EventFlag *getKernelObject(SceUID, int *);
template FPL *getKernelObject(SceUID, int *);
template VPL *getKernelObject(SceUID, int *);

template LwMutex *createKernelObject(int *);
template LwMutex *getKernelObject(SceUID, int *);

template Mutex *createKernelObject(int *);
template Mutex *getKernelObject(SceUID, int *);

template MessageBox *createKernelObject(int *);
template MessageBox *getKernelObject(SceUID, int *);

template Semaphore *createKernelObject(int *);
template Semaphore *getKernelObject(SceUID, int *);

template PSPModule *createKernelObject(int *);
template PSPModule *getKernelObject(SceUID, int *);

template Partition *createKernelObject(int *);
template Partition *getKernelObject(SceUID, int *);

template Callback *createKernelObject(int *);
template Callback *getKernelObject(SceUID, int *);
}