#pragma once

namespace Hydro::Memory
{
    template <typename FunctionType>
    MH_STATUS CreateFunctionHook(void* target, FunctionType* detour, FunctionType** original)
    {
        return MH_CreateHook(target, reinterpret_cast<void*>(detour), reinterpret_cast<void**>(original));
    }

    template <typename FunctionType>
    MH_STATUS CreateExportHook(const wchar_t* moduleName, const char* exportName, FunctionType* detour, FunctionType** original)
    {
        return MH_CreateHookApi(moduleName, exportName, reinterpret_cast<void*>(detour), reinterpret_cast<void**>(original));
    }
}
