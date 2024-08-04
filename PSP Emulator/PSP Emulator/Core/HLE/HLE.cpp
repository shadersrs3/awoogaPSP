#include <Core/HLE/HLE.h>
#include <Core/HLE/RTC.h>

#include <Core/Logger.h>

namespace Core::HLE {
static const char *logType = "HLE";

void __hleInit() {
    rtcInit();
    LOG_INFO(logType, "initialized HLE state");
}

void __hleReset() {
    rtcInit();
    LOG_INFO(logType, "resetted HLE state");
}

void __hleDestroy() {
    LOG_INFO(logType, "destroyed HLE state");
}
}