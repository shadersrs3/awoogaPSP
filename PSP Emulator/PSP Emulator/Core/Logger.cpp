#include <cstdio>
#include <cstdarg>
#include <cinttypes>

#include <map>
#include <mutex>

#include <Core/Timing.h>

#include <Windows.h>

#include <Core/Logger.h>

namespace Core::Logger {
std::map<std::string, int> filteringMap;

static const char *getLogLevelName(int logLevel) {
    switch (logLevel) {
    case LEVEL_TRACE: return "TRACE";
    case LEVEL_INFO: return "INFO";
    case LEVEL_WARNING: return "WARNING";
    case LEVEL_ERROR: return "ERROR";
    case LEVEL_DEBUG: return "DEBUG";
    case LEVEL_SUCCESS: return "SUCCESS";
    case LEVEL_SYSCALL: return "SYSCALL";
    }
    return "?";
}

void setFiltering(const std::string& debugType, Level logLevel) {
    filteringMap[debugType] = logLevel;
}

void resetFiltering() {
    filteringMap.clear();
}
    
bool hasFiltering(const std::string& debugType, Level logLevel) {
    if (const auto& it = filteringMap.find(debugType); it != filteringMap.end()) {
        if (logLevel > it->second)
            return true;
    }
    return false;
}

std::mutex consoleMutex;
HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

void print(const std::string& type, Level level, const std::string& file, int line, std::source_location loc, const char *format, ...) {
    std::lock_guard<std::mutex> l{ consoleMutex };

    if (hasFiltering(type, level) || level == LEVEL_NONE)
        return;

    switch (level) {
    case LEVEL_INFO:
        SetConsoleTextAttribute(hConsole, 0 | FOREGROUND_INTENSITY);
        break;
    case LEVEL_TRACE:
        SetConsoleTextAttribute(hConsole, 7 | FOREGROUND_INTENSITY);
        break;
    case LEVEL_WARNING:
        SetConsoleTextAttribute(hConsole, 6 | FOREGROUND_INTENSITY);
        break;
    case LEVEL_ERROR:
        SetConsoleTextAttribute(hConsole, 4 | FOREGROUND_INTENSITY);
        break;
    case LEVEL_SUCCESS:
        SetConsoleTextAttribute(hConsole, 2 | FOREGROUND_INTENSITY);
        break;
    case LEVEL_SYSCALL:
        SetConsoleTextAttribute(hConsole, 3 | FOREGROUND_INTENSITY);
        break;
    case LEVEL_DEBUG:
        SetConsoleTextAttribute(hConsole, 1 | FOREGROUND_INTENSITY);
        break;
    }

    va_list ap;

    std::string _f;

    size_t x = file.find_last_of('\\');
    if (x == -1) {
        _f = file;
    } else {
        _f = file.substr(x + 1);
    }

    std::string s = std::string(loc.function_name());

    static std::string list[2] = { "__cdecl"};
    for (int i = 0; i < 2; i++) {

        if (auto pos = s.find(list[i]); pos != -1) {
            s = s.substr(pos + list[i].length() + 1);
            s = s.substr(0, s.find('('));
            break;
        }
    }

    uint64_t systemTime = Core::Timing::getSystemTimeMilliseconds();

    uint64_t minutes = ((systemTime / 1000) / 60) % 60;
    uint64_t seconds = (systemTime / 1000) % 60;
    uint64_t millis = (systemTime % 1000);

    std::fprintf(stdout, "[" "%" PRId64 "m:%" PRId64 "s:%" PRId64 "ms] " "[%s:%d] [%s] (%s): ", minutes, seconds, millis, _f.c_str(), line, getLogLevelName(level), type.c_str());
    va_start(ap, format);
    vfprintf(stdout, format, ap);
    va_end(ap);
    std::fprintf(stdout, "\n");
    SetConsoleTextAttribute(hConsole, 7);
}
}