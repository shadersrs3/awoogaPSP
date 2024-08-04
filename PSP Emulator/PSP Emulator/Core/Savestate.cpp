#include <Core/Savestate.h>

#include <Core/Logger.h>

namespace Core::Savestate {
static const char *logType = "savestate";

bool saveState(const std::string& state) {
    LOG_ERROR(logType, "save state unimplemented!");
    return false;
}

bool loadState(const std::string& state) {
    LOG_ERROR(logType, "save state unimplemented!");
    return false;
}
}