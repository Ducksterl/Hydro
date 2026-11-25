#pragma once

namespace Hydro::Memory
{
    class Signature
    {
    public:
        static constexpr std::int16_t AnyByte = -1;

        consteval Signature(std::string_view text)
        {
            while (!text.empty())
            {
                const auto token = text.substr(0, text.find(' '));
                text.remove_prefix(std::min(token.size() + 1, text.size()));

                if (token == "|")
                    MarkerOffset = ByteCount;
                else
                    Bytes[ByteCount++] = token == "?" ? AnyByte : ParseByte(token);
            }
        }

        constexpr std::span<const std::int16_t> GetBytes() const
        {
            return { Bytes.data(), ByteCount };
        }

        constexpr std::size_t GetMarkerOffset() const
        {
            return MarkerOffset;
        }

    private:
        static consteval std::int16_t ParseByte(std::string_view token)
        {
            std::uint8_t value = 0;
            const auto end = token.data() + token.size();
            const auto [parsed, error] = std::from_chars(token.data(), end, value, 16);

            if (token.size() != 2 || parsed != end || error != std::errc{})
                throw "Malformed signature byte";

            return value;
        }

        std::array<std::int16_t, 64> Bytes{};
        std::size_t ByteCount = 0;
        std::size_t MarkerOffset = 0;
    };

    std::uint8_t* FindUniqueMatch(const Signature& signature);
    std::uint8_t* GetCallTarget(std::uint8_t* callInstruction);
}
