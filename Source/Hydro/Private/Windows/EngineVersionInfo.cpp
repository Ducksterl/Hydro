#include "pch.h"
#include "Windows/EngineVersionInfo.h"

namespace UE::VersionInfo
{
    namespace
    {
        struct VersionBlockHeader
        {
            WORD Length;
            WORD ValueLength;
            WORD Type;
        };

        struct VersionBlock
        {
            std::size_t Size;
            std::wstring_view Key;
            std::span<const std::uint8_t> Value;
            std::span<const std::uint8_t> Children;
        };

        constexpr std::size_t AlignVersionOffset(std::size_t offset)
        {
            return (offset + 3) & ~std::size_t{ 3 };
        }

        std::wstring_view ReadWideString(
            std::span<const std::uint8_t> data)
        {
            if (data.size() < sizeof(wchar_t))
                return {};

            const auto* text =
                reinterpret_cast<const wchar_t*>(data.data());

            const auto maxLength = data.size() / sizeof(wchar_t);

            std::size_t length = 0;

            while (length < maxLength && text[length] != L'\0')
                ++length;

            return { text, length };
        }

        std::optional<VersionBlock> ReadVersionBlock(
            std::span<const std::uint8_t> data)
        {
            VersionBlockHeader header{};

            if (data.size() < sizeof(header))
                return std::nullopt;

            std::memcpy(&header, data.data(), sizeof(header));

            if (header.Length < sizeof(header) ||
                header.Length > data.size())
            {
                return std::nullopt;
            }

            const auto block = data.first(header.Length);

            const auto keyData = block.subspan(sizeof(header));
            const auto key = ReadWideString(keyData);

            const auto keySize =
                (key.size() + 1) * sizeof(wchar_t);

            if (keySize > keyData.size())
                return std::nullopt;

            const auto valueOffset = std::min<std::size_t>(
                AlignVersionOffset(sizeof(header) + keySize),
                block.size()
            );

            const std::size_t rawValueSize =
                header.Type == 1
                ? static_cast<std::size_t>(header.ValueLength) * sizeof(wchar_t)
                : static_cast<std::size_t>(header.ValueLength);

            const auto valueSize = std::min<std::size_t>(
                rawValueSize,
                block.size() - valueOffset
            );

            const auto childrenOffset = std::min<std::size_t>(
                AlignVersionOffset(valueOffset + valueSize),
                block.size()
            );

            return VersionBlock
            {
                header.Length,
                key,
                block.subspan(valueOffset, valueSize),
                block.subspan(childrenOffset)
            };
        }

        std::optional<VersionBlock> FindVersionChild(
            const VersionBlock& parent,
            std::wstring_view key)
        {
            auto children = parent.Children;

            while (!children.empty())
            {
                const auto child = ReadVersionBlock(children);

                if (!child)
                    break;

                if (child->Key == key)
                    return child;

                const auto nextOffset =
                    AlignVersionOffset(child->Size);

                if (nextOffset > children.size())
                    break;

                children = children.subspan(nextOffset);
            }

            return std::nullopt;
        }
    }

    std::wstring_view GetProductVersion()
    {
        const auto module = GetModuleHandleW(nullptr);

        const auto resource = FindResourceW(
            module,
            MAKEINTRESOURCEW(VS_VERSION_INFO),
            RT_VERSION
        );

        if (!resource)
            return {};

        const auto loadedResource =
            LoadResource(module, resource);

        if (!loadedResource)
            return {};

        const auto* data = static_cast<const std::uint8_t*>(
            LockResource(loadedResource)
        );

        const auto resourceSize =
            SizeofResource(module, resource);

        if (!data || resourceSize == 0)
            return {};

        const auto root = ReadVersionBlock(
            { data, resourceSize }
        );

        const auto strings = root
            ? FindVersionChild(*root, L"StringFileInfo")
            : std::nullopt;

        const auto table = strings
            ? ReadVersionBlock(strings->Children)
            : std::nullopt;

        const auto product = table
            ? FindVersionChild(*table, L"ProductVersion")
            : std::nullopt;

        return product
            ? ReadWideString(product->Value)
            : std::wstring_view{};
    }

    std::optional<BuildVersion> ParseProductVersion(std::wstring_view productVersion)
    {
        const auto engineEnd = productVersion.find(L'-');

        if (engineEnd == std::wstring_view::npos)
            return std::nullopt;

        const auto changelistEnd = productVersion.find(L'+', engineEnd);

        if (changelistEnd == std::wstring_view::npos)
            return std::nullopt;

        return BuildVersion
        {
            productVersion.substr(0, engineEnd),
            productVersion.substr(engineEnd + 1, changelistEnd - engineEnd - 1),
            productVersion.substr(changelistEnd + 1)
        };
    }
}
