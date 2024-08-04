#include <Core/HLE/Modules/StdioForUser.h>

namespace Core::HLE {
int sceKernelStdin() {
    return 0;
}

int sceKernelStdout() {
    return 1;
}

int sceKernelStderr() {
    return 2;
}
}