#pragma once

namespace Hydro::Network
{
    struct Url
    {
        std::string_view Scheme;
        std::string_view Host;
        std::string_view Port;
        std::string_view PathAndQuery;
    };

    struct ServerAddress
    {
        bool bSecure;
        std::string_view Host;
        std::uint16_t Port;
    };

    constexpr Url ParseUrl(std::string_view url)
    {
        constexpr std::string_view schemeDelimiter = "://";

        const auto schemeEnd = std::min(url.find_first_of(":/?#"), url.size());
        const bool bHasScheme = url.substr(schemeEnd).starts_with(schemeDelimiter);
        const auto authorityStart = bHasScheme ? schemeEnd + schemeDelimiter.size() : 0;
        const auto authorityEnd = std::min(url.find_first_of("/?#", authorityStart), url.size());
        const auto authority = url.substr(authorityStart, authorityEnd - authorityStart);
        const auto hostEnd = std::min(authority.find(':'), authority.size());
        const auto portStart = std::min(hostEnd + 1, authority.size());

        return
        {
            bHasScheme ? url.substr(0, schemeEnd) : std::string_view{},
            authority.substr(0, hostEnd),
            authority.substr(portStart),
            url.substr(authorityEnd)
        };
    }

    consteval ServerAddress ParseServerAddress(std::string_view address)
    {
        constexpr std::string_view hostCharacters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-._";

        const auto url = ParseUrl(address);

        if (url.Host.empty() || url.Host.find_first_not_of(hostCharacters) != std::string_view::npos || !url.PathAndQuery.empty())
            throw "Redirect must be formatted as scheme://host[:port]";

        const bool bSecure = url.Scheme == "https";

        if (!bSecure && url.Scheme != "http")
            throw "Redirect scheme must be http or https";

        std::uint16_t port = bSecure ? 443 : 80;

        if (!url.Port.empty())
        {
            const auto end = url.Port.data() + url.Port.size();
            const auto [parsed, error] = std::from_chars(url.Port.data(), end, port);

            if (parsed != end || error != std::errc{} || port == 0)
                throw "Redirect port is invalid";
        }

        return { bSecure, url.Host, port };
    }
}
