// Test-only fake installer used to prove P3 rollback paths.
// It understands only /DIR= and deliberately damages the target app.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <cwchar>
#include <string>

namespace {
std::wstring unquote(std::wstring value)
{
    if (value.size() >= 2 && value.front() == L'"' && value.back() == L'"')
        return value.substr(1, value.size() - 2);
    return value;
}

std::wstring installDir(int argc, wchar_t **argv)
{
    for (int i = 1; i < argc; ++i) {
        const std::wstring arg(argv[i]);
        if (arg.rfind(L"/DIR=", 0) == 0)
            return unquote(arg.substr(5));
    }
    return {};
}

bool corruptApp(const std::wstring &dir)
{
    const std::wstring path = dir + L"\\SonKuPik-K500.exe";
    HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        return false;
    const char bytes[] = "P3 intentionally corrupted installer fixture";
    DWORD written = 0;
    const bool ok = WriteFile(file, bytes, sizeof(bytes), &written, nullptr) != FALSE;
    CloseHandle(file);
    return ok;
}
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    int argc = 0;
    wchar_t **argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv)
        return 71;
    const std::wstring dir = installDir(argc, argv);
    LocalFree(argv);
    if (dir.empty() || !corruptApp(dir))
        return 72;

    wchar_t mode[32]{};
    GetEnvironmentVariableW(L"SONKUPIK_P3_FAKE_INSTALL_MODE", mode, 32);
    if (std::wstring(mode) == L"nonzero")
        return 7;
    if (std::wstring(mode) == L"badhealth")
        return 0; // Helper must detect that the replaced app can no longer launch.
    return 73;
}
