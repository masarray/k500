// Test-only executable installed by the Inno fixture in GitHub Actions.
// Never ship this file instead of the real K500 application.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <cwchar>
#include <string>

namespace {
void marker(const wchar_t *name)
{
    wchar_t base[MAX_PATH]{};
    const DWORD length = GetTempPathW(MAX_PATH, base);
    if (!length || length >= MAX_PATH)
        return;
    std::wstring path(base);
    path += name;
    HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ,
                              nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file != INVALID_HANDLE_VALUE)
        CloseHandle(file);
}
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    int argc = 0;
    wchar_t **argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv)
        return 51;
    if (argc == 2) {
        const std::wstring argument(argv[1]);
        LocalFree(argv);
        if (argument.rfind(L"--update-health-check=", 0) != 0
            || argument.size() <= std::wcslen(L"--update-health-check="))
            return 52;
        marker(L"SonKuPik-K500-CI-update-health.marker");
        return 0;
    }
    LocalFree(argv);
    if (argc != 1)
        return 53;
    marker(L"SonKuPik-K500-CI-update-relaunch.marker");
    return 0;
}
