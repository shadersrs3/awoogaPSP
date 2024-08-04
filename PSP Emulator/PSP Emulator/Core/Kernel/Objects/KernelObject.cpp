#include <Core/Kernel/Objects/KernelObject.h>

namespace Core::Kernel {
void KernelObject::destroyFromPool() {
    std::vector<int> *v = getKernelObjectList(getType());

    if (!v)
        return;

    std::vector<int>::iterator it = std::find(v->begin(), v->end(), getUID());
    if (it != v->end())
        v->erase(it);
}
}