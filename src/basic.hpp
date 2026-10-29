#pragma once

#include <print>
#include <fstream>
#include <sstream>
#include <string>
#include <cassert>

template<typename... Args>
constexpr void infofn(std::format_string<Args...> fmt, Args&&... args);

template<typename... Args>
constexpr void errorfn(std::format_string<Args...> fmt, Args&&... args);

bool read_entire_file(std::string& file, const char *filename);

///// IMPLEMENTATION ///////////////////////////////////////////////////////////

template<typename... Args>
constexpr void infofn(std::format_string<Args...> fmt, Args&&... args)
{
    std::fputs("[INFO] ", stdout);
    std::println(fmt, std::forward<Args>(args)...);
}

template<typename... Args>
constexpr void errorfn(std::format_string<Args...> fmt, Args&&... args)
{
    std::fputs("[ERROR] ", stderr);
    std::println(stderr, fmt, std::forward<Args>(args)...);
}
