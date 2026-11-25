# Hydro

Hydro is a **Universal Redirect** for Fortnite. It is a DLL that sends the game's backend traffic to a server of your choice. It finds the game code it hooks by signature instead of by offset, so the same DLL works across builds.

## Features

- Redirects every HTTP request the game makes through libcurl or WinINet, keeping the path and query.
- Redirects Epic service hostnames, which covers XMPP and notifications.
- Supports x86 and x64 builds.
- Logs every redirect to the console.

## Supported Versions

| Version | Engine | Architecture | Tested |
| --- | --- | --- | --- |
| `++Fortnite+Release-Live-CL-2870186` | 4.12.0 | x86 | Executable |
| `++Fortnite+Release-Live-CL-3240987` | 4.14.0 | x64 | Executable |
| `++Fortnite+Release-Cert-CL-3532353` | 4.16.0 | x64 | Executable |
| `++Fortnite+Release-Cert-CL-3541083` | 4.16.0 | x64 | Executable |
| `++Fortnite+Release-Cert-CL-3700114` | 4.16.0 | x64 | In-game |

- **In-game:** the game was run with Hydro and worked.
- **Executable:** Hydro's hooks were tested against the code from the game's executable, without running the game.

Hydro may support more versions than the ones listed here. Some versions are unavailable or lost, so they cannot be tested.

## Configuration

The settings are in `pch.h`. Rebuild after changing them.

| Setting | Default | Description |
| --- | --- | --- |
| `bConsole` | `true` | Opens a console with the version and the redirect log |
| `Redirect` | `http://127.0.0.1:3551` | Address of your server |

`Redirect` must be `http://host[:port]` or `https://host[:port]` with no trailing slash or path. An invalid address fails to compile.

## Building

Requires Visual Studio.

1. Open `Hydro.sln`.
2. Select `Release` and either `x86` or `x64`.
3. Build the solution.

| Platform | Output |
| --- | --- |
| x86 | `Release\Hydro.dll` |
| x64 | `x64\Release\Hydro.dll` |

## Usage

1. Start your server on the address set in `Redirect`.
2. Start the game.
3. Inject the `Hydro.dll` that matches the game's architecture. Requests sent before injection are not redirected.

When Hydro has loaded correctly, the console shows:

```
[Hydro] Curl: MH_OK
[Hydro] WinINet: MH_OK
[Hydro] DNS: MH_OK
[Hydro] Console: MH_OK
[Hydro] Hooks: MH_OK
```

## Credits

- [MinHook](https://github.com/TsudaKageyu/minhook) ([license](Source/ThirdParty/MinHook/LICENSE.txt))
