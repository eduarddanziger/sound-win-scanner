# Sound Windows Scanner (SoundWinScanner)

SoundWinScanner is a sound device scanner engine that contains C/C++ sound agent core library with its DLL- and a Go-module-frontends, and a lightweight C# UI to monitor audio endpoint devices.

SoundWinScanner detects and outputs plug-and-play audio endpoint devices under Windows. It handles audio notifications and device changes.

## Modules Generated

- **sound-win-scanner**: Go module that wraps the C++ core logic and provides a Go API for [WinSoundScanner](https://github.com/collect-sound-devices/win-sound-scanner-go)

## Executables Generated

- **SoundDefaultUI**: Lightweight WPF UI showing the live volume levels of the default audio devices, output and input device separately.
  ![SoundDefaultUI screenshot](202509011440SoundDefaultUI.jpg)
- **SoundAgentCli**: Command-line test CLI.

## Technologies Used

- **C++ / Go**: Core logic implementation.
- **RabbitMQ**: Used as a message broker for reliable audio device information delivery.
- **C# / WPF**: Lightweight UI for displaying live volume levels of the currently default audio devices.

## Usage

### SoundDefaultUI
1. Download and unzip the latest rollout of SoundDefaultUI-x.x.x. from the latest repository
release's assets, [Release](https://github.com/collect-sound-devices/sound-win-scanner/releases/latest)

2. Install certificates and unblock the SoundDefaultUI.exe per PowerShell (start as Administrator):

  ```powershell
     Import-Certificate -FilePath .\CodeSign.cer -CertStoreLocation Cert:\LocalMachine\Root
     Unblock-File -Path .\SoundDefaultUI.exe
  ```
3. Run the SoundDefaultUI

## Developer Environment, How to Build:

### Prerequisites

- .NET 10 SDK installed (required to build the WPF `SoundDefaultUI` project).
- vcpkg installed and bootstrapped:
  - Clone vcpkg and run the bootstrap script (Windows): `bootstrap-vcpkg.bat`
  - Ensure `vcpkg.exe` is available on PATH or configure your build to point to it.

### Instructions

1. Install Visual Studio 2026
2. Build the solution, e.g. if you use Visual Studio Community Edition:
```powershell
%NuGet% restore SoundWinScanner.sln
"c:\Program Files\Microsoft Visual Studio\2022\Community\Msbuild\Current\Bin\MSBuild.exe" SoundWinScanner.sln /p:Configuration=Release /target:Rebuild -restore
```

## License

This project is licensed under the terms of the [MIT License](LICENSE).

## Contact

Eduard Danziger

Email: [edanziger@gmx.de](mailto:edanziger@gmx.de)
