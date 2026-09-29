// Test-only fake machine uninstaller for P3 migration qualification.
// It removes the machine app marker and HKLM uninstall registration, then exits.
// It does not model Inno UI/UAC; production code still uses ShellExecute "runas".
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <filesystem>
#include <string>
#include <vector>

#pragma comment(lib, "advapi32.lib")

namespace fs = std::filesystem;

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    std::vector<wchar_t> buffer(32768);
    const DWORD length = GetModuleFileNameW(nullptr, buffer.data(),
                                            static_cast<DWORD>(buffer.size()));
    if (!length || length >= buffer.size())
        return 81;

    const fs::path self(std::wstring(buffer.data(), length));
    const fs::path app = self.parent_path() / L"SonKuPik-K500.exe";
    if (!DeleteFileW(app.c_str()) && GetLastError() != ERROR_FILE_NOT_FOUND)
        return 82;

    constexpr wchar_t parentPath[] =
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall";
    constexpr wchar_t leaf[] =
        L"{8F568FE8-A747-4CD0-A727-5FE81A405500}_is1";

    HKEY parent = nullptr;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, parentPath, 0,
                      KEY_WRITE | KEY_WOW64_64KEY, &parent) != ERROR_SUCCESS)
        return 83;
    const LONG result = RegDeleteTreeW(parent, leaf);
    RegCloseKey(parent);
    if (result != ERROR_SUCCESS && result != ERROR_FILE_NOT_FOUND)
        return 84;
    return 0;
}
