#pragma once

namespace Hydro::Console
{
    void Open(std::wstring_view productVersion);
    void Log(_Printf_format_string_ const char* format, ...);
    MH_STATUS BlockExitEvents();
}
