#include "pch.h"
#include "Memory/Signature.h"

namespace Hydro::Memory
{
    namespace
    {
        std::span<const IMAGE_SECTION_HEADER> GetImageSections(const std::uint8_t* image)
        {
            const auto* dosHeader = reinterpret_cast<const IMAGE_DOS_HEADER*>(image);
            const auto* ntHeaders = reinterpret_cast<const IMAGE_NT_HEADERS*>(image + dosHeader->e_lfanew);

            return { IMAGE_FIRST_SECTION(ntHeaders), ntHeaders->FileHeader.NumberOfSections };
        }

        bool MatchesSignatureByte(std::uint8_t imageByte, std::int16_t signatureByte)
        {
            return signatureByte == Signature::AnyByte || imageByte == signatureByte;
        }
    }

    std::uint8_t* FindUniqueMatch(const Signature& signature)
    {
        auto* image = reinterpret_cast<std::uint8_t*>(GetModuleHandleW(nullptr));
        const auto bytes = signature.GetBytes();

        std::uint8_t* uniqueMatch = nullptr;

        for (const auto& section : GetImageSections(image))
        {
            if ((section.Characteristics & IMAGE_SCN_MEM_EXECUTE) == 0)
                continue;

            auto* match = image + section.VirtualAddress;
            auto* const sectionEnd = match + section.Misc.VirtualSize;

            while ((match = std::search(match, sectionEnd, bytes.begin(), bytes.end(), MatchesSignatureByte)) != sectionEnd)
            {
                if (uniqueMatch)
                    return nullptr;

                uniqueMatch = match++ + signature.GetMarkerOffset();
            }
        }

        return uniqueMatch;
    }

    std::uint8_t* GetCallTarget(std::uint8_t* callInstruction)
    {
        std::int32_t displacement = 0;
        std::memcpy(&displacement, callInstruction + 1, sizeof(displacement));

        return callInstruction + 1 + sizeof(displacement) + displacement;
    }
}
