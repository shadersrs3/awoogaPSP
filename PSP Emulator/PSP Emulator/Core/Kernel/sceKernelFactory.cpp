#include <format>
#include <string>
#include <vector>
#include <unordered_map>

#include <Core/Emulator.h>
#include <Core/PSP/Types.h>
#include <Core/Utility/RandomNumberGenerator.h>

#include <Core/Kernel/sceKernelFactory.h>
#include <Core/Kernel/Objects/KernelObject.h>
#include <Core/Kernel/Objects/Alarm.h>
#include <Core/Kernel/Objects/EventFlag.h>
#include <Core/Kernel/Objects/FPL.h>
#include <Core/Kernel/Objects/LwMutex.h>
#include <Core/Kernel/Objects/MessageBox.h>
#include <Core/Kernel/Objects/Mutex.h>
#include <Core/Kernel/Objects/Semaphore.h>
#include <Core/Kernel/Objects/Thread.h>
#include <Core/Kernel/Objects/VPL.h>
#include <Core/Kernel/Objects/Partition.h>
#include <Core/Kernel/Objects/Callback.h>

#include <Core/Kernel/sceKernelThread.h>
#include <Core/Kernel/sceKernelSystemMemory.h>


#include <Core/Allegrex/AllegrexSyscallHandler.h>

#include <Core/HLE/HLE.h>

#include <Core/Logger.h>

#include "Memory.h"

namespace Core::Kernel {
static const char *logType = "sceKernelFactory";
static const char *logCreation = "sceKernelObjectCreator";

static int64_t kernelObjectsCreated, kernelObjectsDestroyed, currentKernelObjectCount;
static int64_t maxKernelObjects = 2000000;

static std::unordered_map<SceUID, KernelObject *> kernelObjects;

static std::vector<int> kernelThreadObjects, kernelModuleObjects, kernelWaitObjects;

void kernelTraceLogCreation(const char *string, KernelObject *object) {
    std::string fmt;
    if (!object)
        return;

    fmt = std::format("{} uid: 0x{:x}, typename: \"{}\")", string, object->getUID(), object->getTypeName());
    LOG_TRACE(logCreation, fmt.c_str());
}

void kernelDebugLog(const char *string, KernelObject *object) {
    std::string fmt;
    if (!object)
        return;

    fmt = std::format("{} uid: 0x{:x}, typename: \"{}\")", string, object->getUID(), object->getTypeName());
    LOG_DEBUG(logType, fmt.c_str());
}

static void clearKernelObjectList() {
    kernelThreadObjects.clear();
    kernelModuleObjects.clear();
    kernelWaitObjects.clear();
    LOG_TRACE(logType, "cleared kernel object from lists");
}

static void destroyKernelObjectMap() {
    LOG_TRACE(logType, "destroying kernel objects from map...");
    for (auto& i : kernelObjects) {
        auto kobj = i.second;
        if (kobj) {
            kobj->destroyFromPool();
            kernelDebugLog("freeing", kobj);
            delete kobj;
            kobj = nullptr;
        }

        i.second = nullptr;
    }

    kernelObjects.clear();
    currentKernelObjectCount = 0;
}

template<class T>
T *createKernelObject(int *error) {
    T *obj;

    if (currentKernelObjectCount >= maxKernelObjects) {
        if (error)
            *error = SCE_KERNEL_ERROR_NO_MEMORY;

        LOG_ERROR(logType, "reached a maximum number of kernel objects");
        return nullptr;
    }

    obj = new T;

    obj->getUID() = -1;
    obj->getType() = T::getStaticTypeId();

    kernelObjectsCreated++;
    if (error)
        *error = SCE_KERNEL_ERROR_OK;

    return obj;
}

template<typename T>
T *getKernelObject(SceUID uid, int *error) {
    if (auto it = kernelObjects.find(uid); it != kernelObjects.end()) {
        auto kobj = (T *) it->second;
        if (!kobj) {
            if (error)
                *error = SCE_KERNEL_ERROR_UNKNOWN_UID;

            LOG_ERROR(logType, "getting kernel object is null (uid: 0x%x)", uid);
            return nullptr;
        }

        if (kobj->getType() == T::getStaticTypeId()) {
            if (error)
                *error = SCE_KERNEL_ERROR_OK;
            return kobj;
        } else {
            if (error)
                *error = SCE_KERNEL_ERROR_UNKNOWN_UID;

            LOG_ERROR(logType, "getting kernel object is different (requested typename %s, received typename %s, uid 0x%x)",
                kobj->getStaticTypeName(), kobj->getTypeName(), kobj->getUID());
            return nullptr;
        }
    }
    if (error)
        *error = SCE_KERNEL_ERROR_UNKNOWN_UID;
    // LOG_ERROR(logType, "can't get kernel object (uid: 0x%x)", uid);
    return nullptr;
}

template<typename T>
T *createWaitObject(int waitType) {
    auto v = createKernelObject<T>();
    v->setWaitType(waitType);
    saveKernelObject(v);
    return v;
}

void saveKernelObject(KernelObject *object) {
    int uid;
    if (!object) {
        LOG_ERROR(logType, "can't save kernel object! invalid pointer");
        return;
    }

    bool uidIsNotRandom;
    do {
        uid = Core::Utility::getRandomInt();
        uidIsNotRandom = isValidKernelObject(uid);
        if (uidIsNotRandom) {
            LOG_WARN(logType, "saving kernel object is not random retrying..");
        }
    } while (uidIsNotRandom);

    if (std::vector<int> *kernelObjectList = getKernelObjectList(object->getType()); kernelObjectList != nullptr) {
        kernelObjectList->push_back(uid);
    } else {
        LOG_ERROR(logType, "can't find proper object type to assign list (uid: 0x%x, typename: %s)", uid, object->getTypeName());
    }

    object->getUID() = uid;
    kernelObjects[uid] = object;
    currentKernelObjectCount++;
    // kernelTraceLogCreation("saved", object);
}

void destroyKernelObject(SceUID uid, int *error, KernelObjectType type) {
    if (currentKernelObjectCount <= 0) {
        if (error)
            *error = 0xDEADBEEF;
        LOG_ERROR(logType, "can't destroy kernel object (no kernel objects created)");
        return;
    }

    auto it = kernelObjects.find(uid);
    if (it == kernelObjects.end()) {
        if (error)
            *error = 0xDEADBEEF;
        LOG_ERROR(logType, "can't destroy kernel object (uid: 0x%x)", uid);
        return;
    }

    auto object = it->second;
    if (!object) {
        if (error)
            *error = 0xDEADBEEF;
        LOG_ERROR(logType, "can't destroy kernel object (uid: 0x%x)", uid);
        return;
    }

    bool alwaysUnknownType = false;

    if (type == KERNEL_OBJECT_UNKNOWN)
        alwaysUnknownType = true;

    if (!alwaysUnknownType && type != object->getType()) {
        if (error)
            *error = 0xDEADBEEF;

        LOG_ERROR(logType, "destroying kernel object is different (requested typename %s, uid 0x%x)",
            object->getTypeName(), object->getUID());
        return;
    }

    if (object->getType() != KERNEL_OBJECT_WAIT) {
        // kernelTraceLogCreation("destroying", object);
        kernelDebugLog("freeing", object);
    }

    object->destroyFromPool();
    kernelObjects.erase(object->getUID());
    delete object;

    currentKernelObjectCount--;
    kernelObjectsDestroyed++;
    if (error)
        *error = SCE_KERNEL_ERROR_OK;
}

bool isValidKernelObject(SceUID uid) {
    return kernelObjects.find(uid) != kernelObjects.end();
}

std::vector<int> *getKernelObjectList(KernelObjectType type) {
    switch (type) {
    case KERNEL_OBJECT_THREAD: return &kernelThreadObjects;
    case KERNEL_OBJECT_MODULE: return &kernelModuleObjects;
    case KERNEL_OBJECT_WAIT: return &kernelWaitObjects;
    }

    LOG_ERROR(logType, "can't get kernel object list type %d", type);
    return nullptr;
}

static bool kernelInitialized = false;

bool initialize() {
    if (kernelInitialized == true) {
        LOG_ERROR(logType, "can't initialize kernel twice");
        return false;
    }

    // states
    kernelObjectsCreated = 0;
    kernelObjectsDestroyed = 0;
    currentKernelObjectCount = 0;

    // behaviours
    HLE::__hleInit();
    Core::Allegrex::resetSyscallAddressHandler();
    systemMemoryInit();
    kernelThreadInitialize();
    kernelInitialized = true;

    LOG_SUCCESS(logType, "successfully initialized kernel");
    return true;
}

bool reset() {
    if (kernelInitialized == false) {            
        LOG_ERROR(logType, "can't reset kernel while not initialized!");
        return false;
    }

    // states
    kernelObjectsCreated = 0;
    kernelObjectsDestroyed = 0;
    currentKernelObjectCount = 0;

    // behaviours
    Core::Allegrex::resetSyscallAddressHandler();
    destroyKernelObjectMap();
    clearKernelObjectList();
    systemMemoryReset();
    kernelThreadReset();
    Core::HLE::__hleReset();
    LOG_SUCCESS(logType, "successfully resetted kernel");
    return true;
}

bool destroy() {
    if (kernelInitialized == false) {
        LOG_ERROR(logType, "attempting to destroy kernel while not initialized");
        return false;
    }

    // states
    LOG_INFO(logType, "destroying %d kernel objects", currentKernelObjectCount);

    kernelObjectsCreated = 0;
    kernelObjectsDestroyed = 0;
    currentKernelObjectCount = 0;

    // behaviours
    kernelThreadDestroy();
    systemMemoryDestroy();
    destroyKernelObjectMap();
    clearKernelObjectList();
    Core::HLE::__hleDestroy();
    kernelInitialized = false;
    LOG_SUCCESS(logType, "successfully destroyed kernel");
    return true;
}
}