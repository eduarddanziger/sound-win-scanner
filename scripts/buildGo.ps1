<#
.DESCRIPTION
Build the Go test app for Windows with CGO enabled.
This build requires a GNU-compatible Windows toolchain because Go's external
CGO linker path on Windows uses MinGW/GNU-style linker flags and runtime libs.
Use -mingwPath (alias -m) to point at an llvm-mingw or mingw-w64 root, or let
the script auto-detect an existing GNU compiler on PATH.
Use -msbuildConfig (alias -c) to specify the MSBuild configuration, Debug or Release.
.PARAMETER mingwPath
Path to an llvm-mingw or mingw-w64 root. The script will prepend its bin directory to PATH.
.PARAMETER msbuildConfig
MSBuild configuration, Debug or Release.
.PARAMETER respectExistingCompiler
Respect existing CC/CXX values instead of forcing an auto-detected GNU toolchain.
#>

Param(
    [Alias("m")]
    [Parameter(HelpMessage = "Path to an llvm-mingw or mingw-w64 root. The script will prepend its bin directory to PATH.")]
    [string]$mingwPath = "e:\TOOLS\llvm-mingw",
    [Alias("c")]
    [Parameter(HelpMessage = "MSBuild configuration, Debug or Release.")]
    [string]$msbuildConfig = "Release",
    [Parameter(HelpMessage = "Respect existing CC/CXX values instead of forcing an auto-detected GNU toolchain.")]
    [switch]$respectExistingCompiler
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Add-DirectoryToPath {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Directory
    )

    $currentPath = [Environment]::GetEnvironmentVariable("PATH", "Process")
    $pathEntries = @()
    if (-not [string]::IsNullOrWhiteSpace($currentPath)) {
        $pathEntries = $currentPath -split ";"
    }

    $normalizedDirectory = [System.IO.Path]::GetFullPath($Directory).TrimEnd("\")
    foreach ($entry in $pathEntries) {
        if ([string]::IsNullOrWhiteSpace($entry)) {
            continue
        }

        try {
            $normalizedEntry = [System.IO.Path]::GetFullPath($entry).TrimEnd("\")
        } catch {
            continue
        }

        if ($normalizedEntry -ieq $normalizedDirectory) {
            return
        }
    }

    if ([string]::IsNullOrWhiteSpace($currentPath)) {
        [Environment]::SetEnvironmentVariable("PATH", $Directory, "Process")
        return
    }

    [Environment]::SetEnvironmentVariable("PATH", "$Directory;$currentPath", "Process")
}

function Find-CompilerPairOnPath {
    param(
        $CandidatePairs
    )

    foreach ($pair in $CandidatePairs) {
        $cc = Get-Command $pair[0] -ErrorAction SilentlyContinue
        $cxx = Get-Command $pair[1] -ErrorAction SilentlyContinue
        if ($cc -and $cxx) {
            return @{
                CC = $pair[0]
                CXX = $pair[1]
            }
        }
    }

    return $null
}

# go to the repo root (parent of the script directory)
Set-Location -LiteralPath $PSScriptRoot
$repoRoot = [System.IO.Directory]::GetParent($PSScriptRoot).FullName
Set-Location -LiteralPath $repoRoot

if (-not $respectExistingCompiler) {
    if (-not [string]::IsNullOrWhiteSpace($mingwPath)) {
        if (-not (Test-Path -LiteralPath $mingwPath)) {
            throw "mingwPath '$mingwPath' does not exist."
        }

        $mingwBinPath = Join-Path $mingwPath "bin"
        if (-not (Test-Path -LiteralPath $mingwBinPath)) {
            throw "mingwPath '$mingwPath' does not contain a 'bin' directory."
        }

        Add-DirectoryToPath -Directory $mingwBinPath
    }

    $compilerPair = Find-CompilerPairOnPath -CandidatePairs @(
        @("x86_64-w64-mingw32-clang", "x86_64-w64-mingw32-clang++"),
        @("x86_64-w64-mingw32-gcc", "x86_64-w64-mingw32-g++"),
        @("gcc", "g++")
    )

    if (-not $compilerPair) {
        throw "Could not find a GNU-compatible Windows C toolchain. Install llvm-mingw or MSYS2/mingw-w64, or pass -mingwPath. Visual Studio Clang/MSVC alone is not sufficient for this Go CGO build."
    }

    $Env:CC = $compilerPair.CC
    $Env:CXX = $compilerPair.CXX
}

$Env:CGO_ENABLED = "1"
$binPathname = Join-Path $PWD.Path "x64/$msbuildConfig/win-sound-logger.exe"
$includePath = Join-Path $PWD.Path "Projects/SoundAgentApi"
$libPath = Join-Path $repoRoot "x64/$msbuildConfig"
$importLibraryPath = Join-Path $libPath "SoundAgentApi.lib"

if (-not (Test-Path -LiteralPath $importLibraryPath)) {
    throw "Missing '$importLibraryPath'. Build the native solution first for configuration '$msbuildConfig'."
}

New-Item -ItemType Directory -Force -Path (Split-Path -Parent $binPathname) | Out-Null

$env:CGO_CPPFLAGS = "-I `"$includePath`" -DUNICODE -D_UNICODE"
$env:CGO_CFLAGS = "-I `"$includePath`" -DUNICODE -D_UNICODE"
$env:CGO_CXXFLAGS = "-I `"$includePath`" -DUNICODE -D_UNICODE"
$env:CGO_LDFLAGS = "-L `"$libPath`""

go build -o $binPathname .\pkg\win-sound-logger
