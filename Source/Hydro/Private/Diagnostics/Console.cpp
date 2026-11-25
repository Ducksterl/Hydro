#include "pch.h"
#include "Diagnostics/Console.h"
#include "Memory/Hook.h"
#include "Windows/EngineVersionInfo.h"

namespace Hydro::Console
{
    namespace
    {
        std::atomic<bool> bIsOpen = false;

        std::mutex OutputMutex;
        std::condition_variable OutputAvailable;
        std::string QueuedOutput;

        decltype(SetConsoleCtrlHandler)* OriginalSetConsoleCtrlHandler = nullptr;

        std::array<char, 256> BuildTitle(std::wstring_view productVersion)
        {
            std::array<char, 256> title{ "Hydro" };

            if (const auto build = UE::VersionInfo::ParseProductVersion(productVersion))
            {
                std::snprintf(
                    title.data(),
                    title.size(),
                    "Hydro | UE Version: %.*ls | Fortnite Version: %.*ls-CL-%.*ls",
                    static_cast<int>(build->Engine.size()), build->Engine.data(),
                    static_cast<int>(build->Branch.size()), build->Branch.data(),
                    static_cast<int>(build->Changelist.size()), build->Changelist.data()
                );
            }
            else if (!productVersion.empty())
            {
                std::snprintf(
                    title.data(),
                    title.size(),
                    "Hydro | %.*ls",
                    static_cast<int>(productVersion.size()), productVersion.data()
                );
            }

            return title;
        }

        std::string FormatText(const char* format, va_list arguments)
        {
            va_list lengthArguments;
            va_copy(lengthArguments, arguments);
            const auto length = std::vsnprintf(nullptr, 0, format, lengthArguments);
            va_end(lengthArguments);

            std::string text(static_cast<std::size_t>(std::max(length, 0)), '\0');
            std::vsnprintf(text.data(), text.size() + 1, format, arguments);

            return text;
        }

        void WriteQueuedOutput()
        {
            for (;;)
            {
                std::string output;

                {
                    std::unique_lock lock(OutputMutex);
                    OutputAvailable.wait(lock, [] { return !QueuedOutput.empty(); });
                    output.swap(QueuedOutput);
                }

                const auto outputHandle = CreateFileW(L"CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);

                if (outputHandle == INVALID_HANDLE_VALUE)
                    continue;

                DWORD bytesWritten = 0;
                WriteFile(outputHandle, output.data(), static_cast<DWORD>(output.size()), &bytesWritten, nullptr);
                CloseHandle(outputHandle);
            }
        }

        BOOL WINAPI HandleExitEvent(DWORD event)
        {
            if (event == CTRL_CLOSE_EVENT)
            {
                bIsOpen = false;
                FreeConsole();
                ExitThread(0);
            }

            return event == CTRL_C_EVENT || event == CTRL_BREAK_EVENT;
        }

        void PrioritizeExitEventHandler()
        {
            OriginalSetConsoleCtrlHandler(HandleExitEvent, FALSE);
            OriginalSetConsoleCtrlHandler(HandleExitEvent, TRUE);
        }

        BOOL WINAPI SetConsoleCtrlHandlerDetour(PHANDLER_ROUTINE handler, BOOL bAdd)
        {
            const auto bSucceeded = OriginalSetConsoleCtrlHandler(handler, bAdd);

            if (bSucceeded && handler && bAdd)
                PrioritizeExitEventHandler();

            return bSucceeded;
        }
    }

    void Open(std::wstring_view productVersion)
    {
        AllocConsole();
        SetConsoleTitleA(BuildTitle(productVersion).data());

        bIsOpen = true;
        std::thread(WriteQueuedOutput).detach();
    }

    void Log(_Printf_format_string_ const char* format, ...)
    {
        if (!bIsOpen)
            return;

        va_list arguments;
        va_start(arguments, format);
        const auto text = FormatText(format, arguments);
        va_end(arguments);

        {
            const std::scoped_lock lock(OutputMutex);
            QueuedOutput.append("[Hydro] ").append(text).append("\r\n");
        }

        OutputAvailable.notify_one();
    }

    MH_STATUS BlockExitEvents()
    {
        const auto status = Memory::CreateExportHook(L"kernel32.dll", "SetConsoleCtrlHandler", SetConsoleCtrlHandlerDetour, &OriginalSetConsoleCtrlHandler);

        if (status == MH_OK)
            PrioritizeExitEventHandler();

        return status;
    }
}
