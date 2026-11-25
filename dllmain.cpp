#include "pch.h"
#include "Diagnostics/Console.h"
#include "Network/Redirects.h"
#include "Windows/EngineVersionInfo.h"

namespace
{
    DWORD WINAPI Initialize(LPVOID)
    {
        if constexpr (HydroConf::bConsole)
            Hydro::Console::Open(UE::VersionInfo::GetProductVersion());

        if (const auto status = MH_Initialize(); status != MH_OK)
        {
            Hydro::Console::Log("MinHook: %s", MH_StatusToString(status));
            return 0;
        }

        Hydro::Console::Log("Curl: %s", MH_StatusToString(Hydro::Network::RedirectCurl()));
        Hydro::Console::Log("WinINet: %s", MH_StatusToString(Hydro::Network::RedirectWinInet()));
        Hydro::Console::Log("DNS: %s", MH_StatusToString(Hydro::Network::RedirectDns()));

        if constexpr (HydroConf::bConsole)
            Hydro::Console::Log("Console: %s", MH_StatusToString(Hydro::Console::BlockExitEvents()));

        Hydro::Console::Log("Hooks: %s", MH_StatusToString(MH_EnableHook(MH_ALL_HOOKS)));

        return 0;
    }
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        HMODULE pinnedModule = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN | GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, reinterpret_cast<LPCWSTR>(module), &pinnedModule);

        if (const auto thread = CreateThread(nullptr, 0, Initialize, nullptr, 0, nullptr))
            CloseHandle(thread);
    }

    return TRUE;
}
