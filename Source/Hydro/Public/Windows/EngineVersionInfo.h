#pragma once

namespace UE::VersionInfo
{
    struct BuildVersion
    {
        std::wstring_view Engine;
        std::wstring_view Changelist;
        std::wstring_view Branch;
    };

    std::wstring_view GetProductVersion();
    std::optional<BuildVersion> ParseProductVersion(std::wstring_view productVersion);
}
