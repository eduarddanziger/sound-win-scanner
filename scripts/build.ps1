<#
.DESCRIPTION
Build Windows sound binaries and fetch native deps.
Use -mingwPath (alias -m) to point at an llvm-mingw root, or set it to an empty string to leave CC/CXX unchanged.
Use -msbuildConfig (alias -c) to specify the MSBuild configuration, Debug or Release; default is Debug.
.PARAMETER mingwPath
Path to llvm-mingw root; set to empty to skip overriding CC/CXX.
.PARAMETER msbuildConfig
MSBuild configuration, Debug or Release.
#>

Param(
    [Alias("m")]
    [Parameter(HelpMessage = "Path to llvm-mingw root; set to empty to skip overriding CC/CXX.")]
    [string]$mingwPath = "E:\\tools\\llvm-mingw\\",
    [Alias("c")]
    [Parameter(HelpMessage = "MSBuild configuration, Debug or Release.")]
    [string]$msbuildConfig = "Debug"
)

# go to the repo root
# go to the repo root (parent of the script directory)
Set-Location -LiteralPath $PSScriptRoot
$repoRoot = [System.IO.Directory]::GetParent($PSScriptRoot).FullName
Set-Location -LiteralPath $repoRoot

$Env:CGO_ENABLED = "1"
if ($mingwPath -ne "") {
    if (-not (Test-Path -LiteralPath $mingwPath)) {
        Write-Error "mingwPath '$mingwPath' does not exist. Set it to a valid llvm-mingw root or pass an empty string to skip overriding CC/CXX."
        Get-Help $PSCommandPath -Detailed
        exit 1
    }
    $Env:CC = Join-Path $mingwPath "bin/x86_64-w64-mingw32-clang.exe"
    $Env:CXX = Join-Path $mingwPath "bin/x86_64-w64-mingw32-clang++.exe"
}

$binPathname = Join-Path $PWD.Path "x64/$msbuildConfig/win-sound-logger.exe"
$includePath = Join-Path $PWD.Path 'Projects/SoundAgentApi'
$libPath     = Join-Path $repoRoot "x64/$msbuildConfig"

$env:CGO_CPPFLAGS = "-I$includePath -DUNICODE -D_UNICODE"
$env:CGO_CFLAGS   = "-I$includePath -DUNICODE -D_UNICODE"
$env:CGO_CXXFLAGS = "-I$includePath -DUNICODE -D_UNICODE"
$env:CGO_LDFLAGS  = "-L$libPath -lSoundAgentApi"

go build -o $binPathname .\pkg\win-sound-logger
