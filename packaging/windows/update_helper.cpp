// K500_UPDATE_HANDOFF_P1
// Independent Win32 update coordinator: no Qt DLLs, no device I/O, no UAC bypass.
// This executable is copied to the update cache before it is launched because
// the installer replaces the application's original installation directory.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shellapi.h>
#include <bcrypt.h>
#include <array>
#include <climits>
#include <cwchar>
#include <string>
#include <vector>

#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "shell32.lib")

namespace {
constexpr DWORD ParentTimeoutMs = 120000;
constexpr DWORD InstallTimeoutMs = 20 * 60 * 1000;
constexpr DWORD HealthTimeoutMs = 60000;

struct Request {
    DWORD parentPid = 0;
    std::wstring setup;
    std::wstring app;
    std::wstring version;
    std::wstring log;
    std::wstring sha256;
};

std::wstring quote(const std::wstring &value)
{
    std::wstring result = L"\"";
    unsigned slashes = 0;
    for (const wchar_t ch : value) {
        if (ch == L'\\') {
            ++slashes;
        } else if (ch == L'"') {
            result.append(slashes * 2 + 1, L'\\');
            result += ch;
            slashes = 0;
        } else {
            result.append(slashes, L'\\');
            slashes = 0;
            result += ch;
        }
    }
    result.append(slashes * 2, L'\\');
    return result + L"\"";
}

bool parse(int argc, wchar_t **argv, Request &request)
{
    if (argc != 13 || std::wstring(argv[1]) != L"--parent-pid"
        || std::wstring(argv[3]) != L"--setup"
        || std::wstring(argv[5]) != L"--app"
        || std::wstring(argv[7]) != L"--version"
        || std::wstring(argv[9]) != L"--log"
        || std::wstring(argv[11]) != L"--sha256")
        return false;
    wchar_t *end = nullptr;
    const unsigned long pid = std::wcstoul(argv[2], &end, 10);
    if (!pid || !end || *end || pid == ULONG_MAX)
        return false;
    request.parentPid = static_cast<DWORD>(pid);
    request.setup = argv[4];
    request.app = argv[6];
    request.version = argv[8];
    request.log = argv[10];
    request.sha256 = argv[12];
    if (request.setup.empty() || request.app.empty() || request.log.empty()
        || request.version.empty() || request.version.find_first_not_of(L"0123456789.") != std::wstring::npos
        || request.sha256.size() != 64)
        return false;
    for (const wchar_t ch : request.sha256) {
        if (!((ch >= L'0' && ch <= L'9') || (ch >= L'a' && ch <= L'f')))
            return false;
    }
    return true;
}

void logLine(const Request &request, const std::wstring &message)
{
    HANDLE file = CreateFileW(request.log.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ,
                              nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE)
        return;
    SYSTEMTIME time{};
    GetSystemTime(&time);
    wchar_t prefix[96]{};
    swprintf_s(prefix, L"%04d-%02d-%02dT%02d:%02d:%02dZ ",
               time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute, time.wSecond);
    const std::wstring line = std::wstring(prefix) + message + L"\r\n";
    const int byteCount = WideCharToMultiByte(CP_UTF8, 0, line.c_str(),
                                               static_cast<int>(line.size()), nullptr, 0, nullptr, nullptr);
    if (byteCount > 0) {
        std::string utf8(static_cast<size_t>(byteCount), '\0');
        WideCharToMultiByte(CP_UTF8, 0, line.c_str(), static_cast<int>(line.size()),
                            utf8.data(), byteCount, nullptr, nullptr);
        DWORD written = 0;
        WriteFile(file, utf8.data(), static_cast<DWORD>(utf8.size()), &written, nullptr);
    }
    CloseHandle(file);
}

int fail(const Request &request, int code, const std::wstring &message)
{
    logLine(request, L"ERROR " + std::to_wstring(code) + L": " + message);
    MessageBoxW(nullptr, (message + L"\n\nUpdate log:\n" + request.log).c_str(),
                L"SonKuPik K500 update", MB_OK | MB_ICONERROR);
    return code;
}

// Hash while keeping a read handle open with no FILE_SHARE_WRITE. This blocks
// modification of the verified installer between hashing and CreateProcess.
bool verifySetup(const Request &request, HANDLE &lockedFile)
{
    lockedFile = CreateFileW(request.setup.c_str(), GENERIC_READ, FILE_SHARE_READ,
                             nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (lockedFile == INVALID_HANDLE_VALUE)
        return false;
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
    std::array<UCHAR, 32> digest{};
    bool ok = false;
    if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) >= 0) {
        DWORD objectBytes = 0;
        DWORD cbResult = 0;
        if (BCryptGetProperty(algorithm, BCRYPT_OBJECT_LENGTH,
                              reinterpret_cast<PUCHAR>(&objectBytes), sizeof(objectBytes),
                              &cbResult, 0) >= 0 && objectBytes > 0) {
            std::vector<UCHAR> object(objectBytes);
            if (BCryptCreateHash(algorithm, &hash, object.data(), objectBytes, nullptr, 0, 0) >= 0) {
                std::array<UCHAR, 64 * 1024> buffer{};
                DWORD received = 0;
                ok = true;
                for (;;) {
                    if (!ReadFile(lockedFile, buffer.data(), static_cast<DWORD>(buffer.size()),
                                  &received, nullptr)) {
                        ok = false;
                        break;
                    }
                    if (received == 0)
                        break; // Successful EOF.
                    if (BCryptHashData(hash, buffer.data(), received, 0) < 0) {
                        ok = false;
                        break;
                    }
                }
                if (ok && BCryptFinishHash(hash, digest.data(), static_cast<ULONG>(digest.size()), 0) < 0)
                    ok = false;
            }
        }
    }
    if (hash)
        BCryptDestroyHash(hash);
    if (algorithm)
        BCryptCloseAlgorithmProvider(algorithm, 0);
    static constexpr wchar_t hex[] = L"0123456789abcdef";
    std::wstring actual;
    for (const UCHAR value : digest) {
        actual += hex[value >> 4];
        actual += hex[value & 0x0f];
    }
    if (!ok || actual != request.sha256) {
        CloseHandle(lockedFile);
        lockedFile = INVALID_HANDLE_VALUE;
        return false;
    }
    SetFilePointer(lockedFile, 0, nullptr, FILE_BEGIN);
    return true;
}

bool waitParent(const Request &request)
{
    HANDLE parent = OpenProcess(SYNCHRONIZE, FALSE, request.parentPid);
    if (!parent)
        return GetLastError() == ERROR_INVALID_PARAMETER; // Already exited.
    const DWORD wait = WaitForSingleObject(parent, ParentTimeoutMs);
    CloseHandle(parent);
    return wait == WAIT_OBJECT_0;
}

DWORD runAndWait(const std::wstring &exe, const std::wstring &parameters,
                 DWORD timeout, DWORD &exitCode)
{
    std::wstring command = quote(exe) + L" " + parameters;
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(exe.c_str(), command.data(), nullptr, nullptr, FALSE,
                        CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process))
        return GetLastError();
    CloseHandle(process.hThread);
    const DWORD result = WaitForSingleObject(process.hProcess, timeout);
    if (result != WAIT_OBJECT_0) {
        // A timed-out process may still be active. Do not kill it or relaunch.
        CloseHandle(process.hProcess);
        return result == WAIT_TIMEOUT ? ERROR_TIMEOUT : ERROR_GEN_FAILURE;
    }
    if (!GetExitCodeProcess(process.hProcess, &exitCode)) {
        CloseHandle(process.hProcess);
        return ERROR_GEN_FAILURE;
    }
    CloseHandle(process.hProcess);
    return ERROR_SUCCESS;
}

// The helper stays at the original, non-elevated user's integrity level.
// Only the existing machine-wide installer requests administrator permission.
// K500 health checks and relaunch therefore never inherit an elevated token.
DWORD installElevatedAndWait(const Request &request, const std::wstring &parameters,
                             DWORD &exitCode)
{
    SHELLEXECUTEINFOW info{};
    info.cbSize = sizeof(info);
    info.fMask = SEE_MASK_NOCLOSEPROCESS;
    info.lpVerb = L"runas";
    info.lpFile = request.setup.c_str();
    info.lpParameters = parameters.c_str();
    info.nShow = SW_SHOWNORMAL;
    if (!ShellExecuteExW(&info))
        return GetLastError();
    if (!info.hProcess)
        return ERROR_GEN_FAILURE;
    const DWORD result = WaitForSingleObject(info.hProcess, InstallTimeoutMs);
    if (result != WAIT_OBJECT_0) {
        CloseHandle(info.hProcess);
        return result == WAIT_TIMEOUT ? ERROR_TIMEOUT : ERROR_GEN_FAILURE;
    }
    if (!GetExitCodeProcess(info.hProcess, &exitCode)) {
        CloseHandle(info.hProcess);
        return ERROR_GEN_FAILURE;
    }
    CloseHandle(info.hProcess);
    return ERROR_SUCCESS;
}

bool startApplication(const std::wstring &exe)
{
    std::wstring command = quote(exe);
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(exe.c_str(), command.data(), nullptr, nullptr, FALSE,
                        0, nullptr, nullptr, &startup, &process))
        return false;
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return true;
}

bool selfTest()
{
    return quote(L"C:\\Some Folder\\app.exe") == L"\"C:\\Some Folder\\app.exe\""
        && quote(L"C:\\trailing\\") == L"\"C:\\trailing\\\\\""
        && quote(L"has\"quote") == L"\"has\\\"quote\"";
}
} // namespace

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    int argc = 0;
    wchar_t **argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv)
        return 10;
    if (argc == 2 && std::wstring(argv[1]) == L"--self-test") {
        const bool ok = selfTest();
        LocalFree(argv);
        return ok ? 0 : 11;
    }
    Request request;
    const bool valid = parse(argc, argv, request);
    LocalFree(argv);
    if (!valid)
        return 12;

    logLine(request, L"Starting verified update handoff.");
    if (!waitParent(request))
        return fail(request, 13, L"K500 did not exit safely. Installation was not started.");

    HANDLE lockedSetup = INVALID_HANDLE_VALUE;
    if (!verifySetup(request, lockedSetup))
        return fail(request, 14, L"Installer SHA-256 changed after download. Nothing was installed.");

    // /HELPERUPDATE=1 prevents Inno Setup from racing this coordinator to
    // restart the application. Legacy updaters still retain their old [Run] path.
    const std::wstring args = L"/VERYSILENT /SUPPRESSMSGBOXES /NORESTART /CLOSEAPPLICATIONS "
                              L"/AUToupdate=1 /HELPERUPDATE=1 /LOG=" + quote(request.log);
    DWORD installerExit = ~0UL;
    const DWORD launchError = installElevatedAndWait(request, args, installerExit);
    CloseHandle(lockedSetup);
    if (launchError == ERROR_CANCELLED) {
        // UAC cancellation happened before installation. Restore the old app.
        logLine(request, L"Administrator prompt was cancelled; restoring previous app.");
        if (!startApplication(request.app))
            return fail(request, 15, L"Administrator permission was cancelled; reopen K500 manually.");
        return 15;
    }
    if (launchError != ERROR_SUCCESS)
        return fail(request, 15, L"Installer did not finish successfully (Windows error "
                   + std::to_wstring(launchError) + L").");
    if (installerExit != 0)
        return fail(request, 16, L"Installer exit code " + std::to_wstring(installerExit)
                   + L". The previous application was not deliberately deleted by the updater.");

    DWORD healthExit = ~0UL;
    const DWORD healthError = runAndWait(request.app,
        quote(L"--update-health-check=" + request.version), HealthTimeoutMs, healthExit);
    if (healthError != ERROR_SUCCESS || healthExit != 0)
        return fail(request, 17, L"Installed K500 did not pass the version/startup check.");

    logLine(request, L"Installer and app health check passed; launching updated K500.");
    if (!startApplication(request.app))
        return fail(request, 18, L"Updated K500 passed validation but could not be launched.");
    return 0;
}
