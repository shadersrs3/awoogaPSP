#pragma once

#include <cstdio>
#include <string>
#include <source_location>

namespace Core::Logger {
enum Level : int {
    LEVEL_NONE,
    LEVEL_INFO,
    LEVEL_TRACE,
    LEVEL_WARNING,
    LEVEL_ERROR,
    LEVEL_SUCCESS,
    LEVEL_SYSCALL,
    LEVEL_DEBUG,
};

void setFiltering(const std::string& debugType, Level logLevel);
void resetFiltering();
bool hasFiltering(const std::string& debugType, Level logLevel);
void print(const std::string& type, Level level, const std::string& file, int line, std::source_location loc, const char *format = "", ...);
}

#define _CUSTOM_LOG_OUTPUT(type, level, ...) do { Core::Logger::print(type, level, __FILE__, __LINE__, std::source_location::current(), __VA_ARGS__); } while (0)

#define LOG_INFO(type, ...) _CUSTOM_LOG_OUTPUT(type, Core::Logger::LEVEL_INFO, __VA_ARGS__)
#define LOG_TRACE(type, ...) _CUSTOM_LOG_OUTPUT(type, Core::Logger::LEVEL_TRACE, __VA_ARGS__)
#define LOG_WARN(type, ...) _CUSTOM_LOG_OUTPUT(type, Core::Logger::LEVEL_WARNING, __VA_ARGS__)
#define LOG_ERROR(type, ...) _CUSTOM_LOG_OUTPUT(type, Core::Logger::LEVEL_ERROR, __VA_ARGS__)
#define LOG_DEBUG(type, ...) _CUSTOM_LOG_OUTPUT(type, Core::Logger::LEVEL_DEBUG, __VA_ARGS__)
#define LOG_SUCCESS(type, ...) _CUSTOM_LOG_OUTPUT(type, Core::Logger::LEVEL_SUCCESS, __VA_ARGS__)
#define LOG_SYSCALL(type, ...) _CUSTOM_LOG_OUTPUT(type, Core::Logger::LEVEL_SYSCALL, __VA_ARGS__)