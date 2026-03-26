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

# go to the repo root (parent of the script directory)
Set-Location -LiteralPath $PSScriptRoot
$repoRoot = [System.IO.Directory]::GetParent($PSScriptRoot).FullName
Set-Location -LiteralPath $repoRoot

if (-not $respectExistingCompiler) {
    if ($mingwPath -ne "") {
        if (-not (Test-Path -LiteralPath $mingwPath)) {
            Write-Error "mingwPath '$mingwPath' does not exist. Set it to a valid llvm-mingw root or pass an empty string to skip overriding CC/CXX."
            Get-Help $PSCommandPath -Detailed
            exit 1
        }
        $Env:CC = Join-Path $mingwPath "bin/x86_64-w64-mingw32-clang.exe"
        $Env:CXX = Join-Path $mingwPath "bin/x86_64-w64-mingw32-clang++.exe"
    }
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
