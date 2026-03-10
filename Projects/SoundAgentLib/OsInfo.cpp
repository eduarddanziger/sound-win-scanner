// ReSharper disable once CppUnusedIncludeDirective
#include "os-dependencies.h"

#include <spdlog/spdlog.h>
#include <winternl.h>


#include "OsInfo.h"

std::string ed::audio::GetOperationSystemName()
{
    constexpr auto windowsNoVersionInfo = "Windows, no version info";

    OSVERSIONINFOEX osVersionInfo{};
    osVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

    typedef NTSTATUS(WINAPI* RtlGetVersionFuncT)(OSVERSIONINFOEX*);
    const auto ntDllHandle = GetModuleHandleA("ntdll.dll");
    if (ntDllHandle == nullptr)
    {
        spdlog::warn("Can not get operating system version info: ntdll.dll is unavailable.");
        return windowsNoVersionInfo;
    }

    const auto rtlGetVersionFuncPtr =
        reinterpret_cast<RtlGetVersionFuncT>(GetProcAddress(ntDllHandle, "RtlGetVersion"));  // NOLINT(clang-diagnostic-cast-function-type-strict)
    if (rtlGetVersionFuncPtr == nullptr)
    {
        spdlog::warn("Can not get operating system version info: RtlGetVersion is unavailable.");
        return windowsNoVersionInfo;
    }

    if (const auto rtlGetVersionResult = rtlGetVersionFuncPtr(&osVersionInfo); rtlGetVersionResult != 0)
    {
        spdlog::warn("Can not get operating system version info: RtlGetVersion failed with status {}.", rtlGetVersionResult);
        return windowsNoVersionInfo;
    }

    HKEY hKey = nullptr;
    if (const auto regOpenResult = RegOpenKeyExA(HKEY_LOCAL_MACHINE, R"(SOFTWARE\Microsoft\Windows NT\CurrentVersion)", 0, KEY_READ, &hKey);
        regOpenResult != ERROR_SUCCESS)
    {
        spdlog::warn("Can not get operating system version info: registry key SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion open failed with status {}.", regOpenResult);
        return windowsNoVersionInfo;
    }
    char productName[256]{};
    DWORD osRevision{ 0 };
    char displayVersion[256]{};
    // ReSharper disable once CppLocalVariableMayBeConst
    char editionId[256]{};

    // Get Product Name
    DWORD size = sizeof(productName);
    if (RegQueryValueExA(hKey, "ProductName", nullptr, nullptr,
        reinterpret_cast<LPBYTE>(productName), &size) == ERROR_SUCCESS) {
        // Successfully got product name
    }
    else
    {
        spdlog::info("Can not extend operating system version info: registry query for ProductName for registry key SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion failed, ignore it.");
    }

    // Get UBR (Update Build Revision)
    size = sizeof(osRevision);
    if (RegQueryValueExA(hKey, "UBR", nullptr, nullptr,
        reinterpret_cast<LPBYTE>(&osRevision), &size) == ERROR_SUCCESS) {
    }
    else
    {
        spdlog::info("Can not extend operating system version info: registry query for UBR (update-build-revision) for registry key SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion failed, ignore it.");
    }

    // Get DisplayVersion (for Win11 22H2+)
    size = sizeof(displayVersion);
    if (RegQueryValueExA(hKey, "DisplayVersion", nullptr, nullptr,
        reinterpret_cast<LPBYTE>(displayVersion), &size) == ERROR_SUCCESS) {
    }
    else
    {
        spdlog::info("Can not extend operating system version info: registry query for DisplayVersion for registry key SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion failed, ignore it.");
    }

    // Get EditionID (Pro, Home, etc.)
    size = sizeof(editionId);
    if (RegQueryValueExA(hKey, "EditionID", nullptr, nullptr,
        reinterpret_cast<LPBYTE>(editionId), &size) == ERROR_SUCCESS) {
    }
    else
    {
        spdlog::info("Can not get operating system version info: registry query for EditionID for registry key SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion failed, ignore it.");
    }

    RegCloseKey(hKey);

    // Windows 11 detection (build 22000+ or product name contains "Windows 11")
    bool isWindows11 = false;
    if (osVersionInfo.dwBuildNumber >= 22000 ||
        strstr(productName, "Windows 11") != nullptr)
    {
        isWindows11 = true;
    }

    // Format the version string
    std::string versionString;
    if (isWindows11)
    {
        versionString = std::string("Windows 11 ") + editionId;
    }
    else
    {
        versionString = std::string(productName);
    }

    // Add display version if available (e.g., "22H2")
    if (strlen(displayVersion) > 0) {
        versionString += " " + std::string(displayVersion);
    }

    versionString += " Build " + std::to_string(osVersionInfo.dwBuildNumber) + "." + std::to_string(osRevision);

    return versionString;
}
