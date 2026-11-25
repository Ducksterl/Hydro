#include "pch.h"
#include "Network/Redirects.h"
#include "Diagnostics/Console.h"
#include "Memory/Hook.h"
#include "Memory/Signature.h"
#include "Network/Url.h"
#include <WinInet.h>
#include <WS2tcpip.h>

namespace Hydro::Network
{
    namespace
    {
        using CurlSetOptFunction = int __cdecl(void* handle, int option, va_list arguments);

        constexpr int CurlOptionUrl = 10002;

        constexpr std::array CurlEasySetOptSignatures
        {
#ifdef _WIN64
            Memory::Signature{ "89 54 24 10 4C 89 44 24 18 4C 89 4C 24 20 48 83 EC 28 48 85 C9 75 08 8D 41 2B 48 83 C4 28 C3 4C 8D 44 24 40 | E8 ? ? ? ? 48 83 C4 28 C3" }
#else
            Memory::Signature{ "8B 44 24 04 85 C0 75 06 B8 2B 00 00 00 C3 8D 4C 24 0C 51 FF 74 24 0C 50 | E8 ? ? ? ? 83 C4 0C C3" }
#endif
        };

        constexpr std::array<std::string_view, 4> EpicHostSuffixes
        {
            ".epicgames.com",
            ".epicgames.net",
            "cdn1.unrealengine.com",
            "launcher-resources.s3.amazonaws.com"
        };

        constexpr ServerAddress Server = ParseServerAddress(HydroConf::Redirect);

        const std::string ServerHost(Server.Host);
        const std::wstring WideServerHost(Server.Host.begin(), Server.Host.end());

        CurlSetOptFunction* OriginalCurlSetOpt = nullptr;
        decltype(InternetConnectW)* OriginalInternetConnect = nullptr;
        decltype(HttpOpenRequestW)* OriginalHttpOpenRequest = nullptr;
        decltype(getaddrinfo)* OriginalGetAddrInfo = nullptr;

        std::mutex RedirectedConnectionsMutex;
        std::unordered_set<HINTERNET> RedirectedConnections;

        std::optional<std::string> BuildRedirectedUrl(std::string_view originalUrl)
        {
            const auto url = ParseUrl(originalUrl);

            if (url.Host.empty())
                return std::nullopt;

            return std::string(HydroConf::Redirect).append(url.PathAndQuery);
        }

        int __cdecl CallOriginalCurlSetOpt(void* handle, int option, ...)
        {
            va_list arguments;
            va_start(arguments, option);
            const auto result = OriginalCurlSetOpt(handle, option, arguments);
            va_end(arguments);

            return result;
        }

        int __cdecl CurlSetOptDetour(void* handle, int option, va_list arguments)
        {
            if (option == CurlOptionUrl)
            {
                va_list urlArguments;
                va_copy(urlArguments, arguments);
                const auto* originalUrl = va_arg(urlArguments, const char*);
                va_end(urlArguments);

                if (const auto redirectedUrl = originalUrl ? BuildRedirectedUrl(originalUrl) : std::nullopt)
                {
                    Console::Log("%s -> %s", originalUrl, redirectedUrl->c_str());
                    return CallOriginalCurlSetOpt(handle, option, redirectedUrl->c_str());
                }
            }

            return OriginalCurlSetOpt(handle, option, arguments);
        }

        HINTERNET WINAPI InternetConnectDetour(HINTERNET internet, LPCWSTR host, INTERNET_PORT port, LPCWSTR userName, LPCWSTR password, DWORD service, DWORD flags, DWORD_PTR context)
        {
            if (service != INTERNET_SERVICE_HTTP || !host || !*host)
                return OriginalInternetConnect(internet, host, port, userName, password, service, flags, context);

            Console::Log("%ls:%u -> %ls:%u", host, port, WideServerHost.c_str(), Server.Port);

            const auto connection = OriginalInternetConnect(internet, WideServerHost.c_str(), Server.Port, userName, password, service, flags, context);

            if (connection)
            {
                const std::scoped_lock lock(RedirectedConnectionsMutex);
                RedirectedConnections.insert(connection);
            }

            return connection;
        }

        bool IsRedirectedConnection(HINTERNET connection)
        {
            const std::scoped_lock lock(RedirectedConnectionsMutex);
            return RedirectedConnections.contains(connection);
        }

        HINTERNET WINAPI HttpOpenRequestDetour(HINTERNET connection, LPCWSTR verb, LPCWSTR objectName, LPCWSTR version, LPCWSTR referrer, LPCWSTR* acceptTypes, DWORD flags, DWORD_PTR context)
        {
            if (IsRedirectedConnection(connection))
            {
                flags &= ~INTERNET_FLAG_SECURE;

                if constexpr (Server.bSecure)
                    flags |= INTERNET_FLAG_SECURE;
            }

            return OriginalHttpOpenRequest(connection, verb, objectName, version, referrer, acceptTypes, flags, context);
        }

        bool IsEpicHost(std::string_view host)
        {
            if (host.ends_with('.'))
                host.remove_suffix(1);

            return std::ranges::any_of(EpicHostSuffixes, [host](std::string_view suffix)
            {
                return host.size() >= suffix.size() && _strnicmp(host.data() + host.size() - suffix.size(), suffix.data(), suffix.size()) == 0;
            });
        }

        INT WSAAPI GetAddrInfoDetour(PCSTR host, PCSTR service, const ADDRINFOA* hints, PADDRINFOA* result)
        {
            if (!host || !IsEpicHost(host))
                return OriginalGetAddrInfo(host, service, hints, result);

            Console::Log("%s -> %s", host, ServerHost.c_str());
            return OriginalGetAddrInfo(ServerHost.c_str(), service, hints, result);
        }
    }

    MH_STATUS RedirectCurl()
    {
        for (const auto& signature : CurlEasySetOptSignatures)
        {
            if (auto* callInstruction = Memory::FindUniqueMatch(signature))
                return Memory::CreateFunctionHook(Memory::GetCallTarget(callInstruction), CurlSetOptDetour, &OriginalCurlSetOpt);
        }

        return MH_ERROR_FUNCTION_NOT_FOUND;
    }

    MH_STATUS RedirectWinInet()
    {
        if (const auto status = Memory::CreateExportHook(L"wininet.dll", "HttpOpenRequestW", HttpOpenRequestDetour, &OriginalHttpOpenRequest); status != MH_OK)
            return status;

        return Memory::CreateExportHook(L"wininet.dll", "InternetConnectW", InternetConnectDetour, &OriginalInternetConnect);
    }

    MH_STATUS RedirectDns()
    {
        return Memory::CreateExportHook(L"ws2_32.dll", "getaddrinfo", GetAddrInfoDetour, &OriginalGetAddrInfo);
    }
}
