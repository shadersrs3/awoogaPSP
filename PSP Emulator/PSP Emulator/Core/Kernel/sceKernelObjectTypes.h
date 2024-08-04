#pragma once

namespace Core::Kernel {
enum KernelObjectType : int {
    KERNEL_OBJECT_ALARM,
    KERNEL_OBJECT_EVENT_FLAG,
    KERNEL_OBJECT_FPL,
    KERNEL_OBJECT_LW_MUTEX,
    KERNEL_OBJECT_MESSAGE_BOX,
    KERNEL_OBJECT_MUTEX,
    KERNEL_OBJECT_SEMAPHORE,
    KERNEL_OBJECT_THREAD,
    KERNEL_OBJECT_VPL,
    KERNEL_OBJECT_WAIT,
    KERNEL_OBJECT_MODULE,
    KERNEL_OBJECT_WAIT_OBJECT,
    KERNEL_OBJECT_PARTITION,
    KERNEL_OBJECT_CALLBACK,
    KERNEL_OBJECT_UNKNOWN,
};
}