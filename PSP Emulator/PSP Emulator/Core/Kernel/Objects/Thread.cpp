#include <algorithm>
#include <vector>

#include <Core/Allegrex/CPURegisterName.h>

#include <Core/Kernel/sceKernelFactory.h>
#include <Core/Kernel/Objects/Thread.h>

using namespace Core::Allegrex;

namespace Core::Kernel {
void PSPThread::resetContext() {
    context.gpr[0] = 0;
    context.fpr[0] = 0x7F800001;
    for (int i = 1; i < 32; i++) {
        context.gpr[i] = 0xDEADBEEF;
        context.fpr[i] = 0x7F800001;
    }

    for (int i = 0; i < 128; i++)
        context.vpr[i] = 0x7F800001;

    for (int i = 0; i < 16; i++) {
        context.vfpuCtrl[i] = 0x00000000;
    }

    context.vfpuCtrl[VFPU_CTRL_SPREFIX] = 0xe4;
    context.vfpuCtrl[VFPU_CTRL_TPREFIX] = 0xe4;
    context.vfpuCtrl[VFPU_CTRL_DPREFIX] = 0x0;
    context.vfpuCtrl[VFPU_CTRL_CC] = 0x3f;
    context.vfpuCtrl[VFPU_CTRL_INF4] = 0;
    context.vfpuCtrl[VFPU_CTRL_REV] = 0x7772ceab;
    context.vfpuCtrl[VFPU_CTRL_RCX0] = 0x3f800001;
    context.vfpuCtrl[VFPU_CTRL_RCX1] = 0x3f800002;
    context.vfpuCtrl[VFPU_CTRL_RCX2] = 0x3f800004;
    context.vfpuCtrl[VFPU_CTRL_RCX3] = 0x3f800008;
    context.vfpuCtrl[VFPU_CTRL_RCX4] = 0x3f800000;
    context.vfpuCtrl[VFPU_CTRL_RCX5] = 0x3f800000;
    context.vfpuCtrl[VFPU_CTRL_RCX6] = 0x3f800000;
    context.vfpuCtrl[VFPU_CTRL_RCX7] = 0x3f800000;

    context.hi = context.lo = 0xDEADBEEF;
    context.pc = 0xFFFFFFFF;
    context.fpcond = false;
    context.fcr31 = 0xE00;
    context.llBit = false;

    std::memset(waitData, 0, sizeof waitData);
    std::memset(stackFrame, 0, sizeof stackFrame);
    stackFrameIndex = 0;
}
}