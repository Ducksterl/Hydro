#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <Windows.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <charconv>
#include <condition_variable>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_set>
#include "MinHook.h"

namespace HydroConf
{
    inline constexpr bool bConsole = true;
    inline constexpr std::string_view Redirect = "http://127.0.0.1:3551";
}
