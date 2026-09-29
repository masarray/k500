// K500_UPDATE_HANDOFF_P3
// Independent Win32 update coordinator: no Qt DLLs, no device I/O, no UAC bypass.
// It owns verified install handoff, executable rollback, and explicit machine->user
// migration. It is staged outside the installation directory before execution.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shellapi.h>
#include <bcrypt.h>
#include <array>
#include <climits>
#include <cstdlib>
#include <cwchar>
#include <filesystem>
#include <fstream>
#include <cstdint>
#include <string>
#include <system_error>
#include <vector>

#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "advapi32.lib")

namespace {
namespace fs = std::filesystem;

constexpr DWORD ParentTimeoutMs = 120000;
constexpr DWORD InstallTimeoutMs = 20 * 60 * 1000;
constexpr DWORD HealthTimeoutMs = 60000;
constexpr wchar_t AppExeName[] = L"SonKuPik-K500.exe";
constexpr wchar_t UninstallerName[] = L"unins000.exe";
constexpr wchar_t UninstallKey[] =
    L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\"
    L"{8F568FE8-A747-4CD0-A727-5FE81A405500}_is1";

struct Request {
    DWORD parentPid = 0;
    std::wstring setup;
    std::wstring app;              // current installed app
    std::wstring targetApp;        // post-update app (different only for migration)
    std::wstring version;          // target version
    std::wstring previousVersion;  // rollback health-check version
    std::wstring log;
    std::wstring sha256;
    std::wstring scope;            // target scope: machine/user
    std::wstring mode;             // update/migrate
    std::wstring backup;           // recovery snapshot directory
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

std::wstring parentDirectory(const std::wstring &path)
{
    const size_t separator = path.find_last_of(L"\\/");
    if (separator == std::wstring::npos || separator == 0)
        return {};
    return path.substr(0, separator);
}

bool validVersion(const std::wstring &version)
{
    return !version.empty()
        && version.find_first_not_of(L"0123456789.") == std::wstring::npos;
}

bool validSha256(const std::wstring &value)
{
    if (value.size() != 64)
        return false;
    for (const wchar_t ch : value) {
        if (!((ch >= L'0' && ch <= L'9') || (ch >= L'a' && ch <= L'f')))
            return false;
    }
    return true;
}

bool parse(int argc, wchar_t **argv, Request &request)
{
    if (argc != 23
        || std::wstring(argv[1]) != L"--parent-pid"
        || std::wstring(argv[3]) != L"--setup"
        || std::wstring(argv[5]) != L"--app"
        || std::wstring(argv[7]) != L"--target-app"
        || std::wstring(argv[9]) != L"--version"
        || std::wstring(argv[11]) != L"--previous-version"
        || std::wstring(argv[13]) != L"--log"
        || std::wstring(argv[15]) != L"--sha256"
        || std::wstring(argv[17]) != L"--scope"
        || std::wstring(argv[19]) != L"--mode"
        || std::wstring(argv[21]) != L"--backup")
        return false;

    wchar_t *end = nullptr;
    const unsigned long pid = std::wcstoul(argv[2], &end, 10);
    if (!pid || !end || *end || pid == ULONG_MAX)
        return false;

    request.parentPid = static_cast<DWORD>(pid);
    request.setup = argv[4];
    request.app = argv[6];
    request.targetApp = argv[8];
    request.version = argv[10];
    request.previousVersion = argv[12];
    request.log = argv[14];
    request.sha256 = argv[16];
    request.scope = argv[18];
    request.mode = argv[20];
    request.backup = argv[22];

    if (request.setup.empty() || request.app.empty() || request.targetApp.empty()
        || request.log.empty() || request.backup.empty()
        || !validVersion(request.version) || !validVersion(request.previousVersion)
        || !validSha256(request.sha256)
        || (request.scope != L"machine" && request.scope != L"user")
        || (request.mode != L"update" && request.mode != L"migrate"))
        return false;

    if (request.mode == L"migrate" && request.scope != L"user")
        return false;
    if (request.mode == L"update" && request.app != request.targetApp)
        return false;
    return true;
}

bool testMode()
{
    wchar_t value[8]{};
    const DWORD size = GetEnvironmentVariableW(L"SONKUPIK_UPDATE_HELPER_TEST", value, 8);
    return size == 1 && value[0] == L'1';
}

void appendLog(const std::wstring &path, const std::wstring &message)
{
    HANDLE file = CreateFileW(path.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ,
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

void logLine(const Request &request, const std::wstring &message)
{
    appendLog(request.log, message);
}

int fail(const Request &request, int code, const std::wstring &message)
{
    logLine(request, L"ERROR " + std::to_wstring(code) + L": " + message);
    if (!testMode()) {
        MessageBoxW(nullptr, (message + L"\n\nUpdate log:\n" + request.log).c_str(),
                    L"SonKuPik K500 update", MB_OK | MB_ICONERROR);
    }
    return code;
}

// Hash while keeping a read handle open with no FILE_SHARE_WRITE. This blocks
// modification of the verified installer between hashing and process creation.
bool verifySetup(const Request &request, HANDLE &lockedFile)
{
    lockedFile = CreateFileW(request.setup.c_str(), GENERIC_READ, FILE_SHARE_READ,
                             nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (lockedFile == INVALID_HANDLE_VALUE)
        return false;

    BCRYPT_ALG_HANDLE algorithm = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
    std::array<UCHAR, 32> digest{};
    std::vector<UCHAR> hashObject;
    bool ok = false;

    if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) >= 0) {
        DWORD objectBytes = 0;
        DWORD cbResult = 0;
        if (BCryptGetProperty(algorithm, BCRYPT_OBJECT_LENGTH,
                              reinterpret_cast<PUCHAR>(&objectBytes), sizeof(objectBytes),
                              &cbResult, 0) >= 0 && objectBytes > 0) {
            hashObject.resize(objectBytes);
            if (BCryptCreateHash(algorithm, &hash, hashObject.data(), objectBytes, nullptr, 0, 0) >= 0) {
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
                        break;
                    if (BCryptHashData(hash, buffer.data(), received, 0) < 0) {
                        ok = false;
                        break;
                    }
                }
                if (ok && BCryptFinishHash(hash, digest.data(),
                                            static_cast<ULONG>(digest.size()), 0) < 0)
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
        return GetLastError() == ERROR_INVALID_PARAMETER;
    const DWORD wait = WaitForSingleObject(parent, ParentTimeoutMs);
    CloseHandle(parent);
    return wait == WAIT_OBJECT_0;
}

DWORD runAndWait(const std::wstring &exe, const std::wstring &parameters,
                 DWORD timeout, DWORD &exitCode)
{
    std::wstring command = quote(exe);
    if (!parameters.empty())
        command += L" " + parameters;
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(exe.c_str(), command.data(), nullptr, nullptr, FALSE,
                        CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process))
        return GetLastError();
    CloseHandle(process.hThread);
    const DWORD result = WaitForSingleObject(process.hProcess, timeout);
    if (result != WAIT_OBJECT_0) {
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

DWORD runElevatedAndWait(const std::wstring &exe, const std::wstring &parameters,
                         DWORD timeout, DWORD &exitCode)
{
    SHELLEXECUTEINFOW info{};
    info.cbSize = sizeof(info);
    info.fMask = SEE_MASK_NOCLOSEPROCESS;
    info.lpVerb = L"runas";
    info.lpFile = exe.c_str();
    info.lpParameters = parameters.empty() ? nullptr : parameters.c_str();
    info.nShow = SW_SHOWNORMAL;
    if (!ShellExecuteExW(&info))
        return GetLastError();
    if (!info.hProcess)
        return ERROR_GEN_FAILURE;
    const DWORD result = WaitForSingleObject(info.hProcess, timeout);
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

bool pathExists(const std::wstring &path)
{
    return GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

bool samePath(const std::wstring &left, const std::wstring &right)
{
    return _wcsicmp(left.c_str(), right.c_str()) == 0;
}

bool readRegisteredPath(HKEY root, std::wstring &path)
{
    HKEY key = nullptr;
    const REGSAM view = root == HKEY_LOCAL_MACHINE ? KEY_WOW64_64KEY : 0;
    if (RegOpenKeyExW(root, UninstallKey, 0, KEY_READ | view, &key) != ERROR_SUCCESS)
        return false;
    wchar_t buffer[32768]{};
    DWORD type = 0;
    DWORD bytes = sizeof(buffer);
    const LONG result = RegQueryValueExW(
        key, L"Inno Setup: App Path", nullptr, &type,
        reinterpret_cast<LPBYTE>(buffer), &bytes);
    RegCloseKey(key);
    if (result != ERROR_SUCCESS || (type != REG_SZ && type != REG_EXPAND_SZ))
        return false;
    path.assign(buffer);
    return !path.empty();
}

struct RegistryValueSnapshot {
    std::wstring name;
    DWORD type = REG_NONE;
    std::vector<BYTE> data;
};

HKEY rootForScope(const std::wstring &scope)
{
    return scope == L"machine" ? HKEY_LOCAL_MACHINE
        : scope == L"user" ? HKEY_CURRENT_USER : nullptr;
}

REGSAM viewForRoot(HKEY root)
{
    return root == HKEY_LOCAL_MACHINE ? KEY_WOW64_64KEY : 0;
}

std::wstring registrySnapshotPath(const std::wstring &backup)
{
    return (fs::path(backup) / L"uninstall-registry.bin").wstring();
}

bool snapshotUninstallRegistry(const std::wstring &scope,
                               const std::wstring &backup,
                               std::wstring *error)
{
    const HKEY root = rootForScope(scope);
    if (!root) {
        if (error) *error = L"Unknown registry scope.";
        return false;
    }
    HKEY key = nullptr;
    if (RegOpenKeyExW(root, UninstallKey, 0, KEY_READ | viewForRoot(root), &key) != ERROR_SUCCESS) {
        if (error) *error = L"Installed uninstall registration could not be opened.";
        return false;
    }

    DWORD subKeys = 0, values = 0, maxName = 0, maxData = 0;
    const LONG info = RegQueryInfoKeyW(key, nullptr, nullptr, nullptr, &subKeys, nullptr, nullptr,
                                      &values, &maxName, &maxData, nullptr, nullptr);
    if (info != ERROR_SUCCESS || subKeys != 0 || values > 512
        || maxName > 32767 || maxData > 16 * 1024 * 1024) {
        RegCloseKey(key);
        if (error) *error = L"Uninstall registration shape is not safe to snapshot.";
        return false;
    }

    std::vector<RegistryValueSnapshot> records;
    records.reserve(values);
    for (DWORD index = 0; index < values; ++index) {
        std::vector<wchar_t> name(static_cast<size_t>(maxName) + 2, L'\0');
        DWORD nameChars = static_cast<DWORD>(name.size());
        std::vector<BYTE> data(static_cast<size_t>(maxData) + 2);
        DWORD dataBytes = static_cast<DWORD>(data.size());
        DWORD type = REG_NONE;
        const LONG result = RegEnumValueW(key, index, name.data(), &nameChars, nullptr,
                                          &type, data.data(), &dataBytes);
        if (result != ERROR_SUCCESS) {
            RegCloseKey(key);
            if (error) *error = L"Could not enumerate uninstall registration.";
            return false;
        }
        name.resize(nameChars);
        data.resize(dataBytes);
        records.push_back({std::wstring(name.begin(), name.end()), type, std::move(data)});
    }
    RegCloseKey(key);

    std::ofstream out(fs::path(registrySnapshotPath(backup)),
                      std::ios::binary | std::ios::trunc);
    if (!out) {
        if (error) *error = L"Could not create uninstall-registration recovery snapshot.";
        return false;
    }
    const uint32_t magic = 0x3352474B; // "KGR3"
    const uint32_t count = static_cast<uint32_t>(records.size());
    out.write(reinterpret_cast<const char *>(&magic), sizeof(magic));
    out.write(reinterpret_cast<const char *>(&count), sizeof(count));
    for (const auto &record : records) {
        const uint32_t type = record.type;
        const uint32_t nameChars = static_cast<uint32_t>(record.name.size());
        const uint32_t dataBytes = static_cast<uint32_t>(record.data.size());
        out.write(reinterpret_cast<const char *>(&type), sizeof(type));
        out.write(reinterpret_cast<const char *>(&nameChars), sizeof(nameChars));
        out.write(reinterpret_cast<const char *>(&dataBytes), sizeof(dataBytes));
        out.write(reinterpret_cast<const char *>(record.name.data()),
                  static_cast<std::streamsize>(nameChars * sizeof(wchar_t)));
        out.write(reinterpret_cast<const char *>(record.data.data()),
                  static_cast<std::streamsize>(dataBytes));
    }
    if (!out.good()) {
        if (error) *error = L"Could not finish uninstall-registration recovery snapshot.";
        return false;
    }
    return true;
}

bool readRegistrySnapshot(const std::wstring &backup,
                          std::vector<RegistryValueSnapshot> &records,
                          std::wstring *error)
{
    std::ifstream in(fs::path(registrySnapshotPath(backup)), std::ios::binary);
    if (!in) {
        if (error) *error = L"Uninstall-registration recovery snapshot is missing.";
        return false;
    }
    uint32_t magic = 0, count = 0;
    in.read(reinterpret_cast<char *>(&magic), sizeof(magic));
    in.read(reinterpret_cast<char *>(&count), sizeof(count));
    if (!in || magic != 0x3352474B || count > 512) {
        if (error) *error = L"Uninstall-registration recovery snapshot is invalid.";
        return false;
    }
    records.clear();
    records.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        uint32_t type = 0, nameChars = 0, dataBytes = 0;
        in.read(reinterpret_cast<char *>(&type), sizeof(type));
        in.read(reinterpret_cast<char *>(&nameChars), sizeof(nameChars));
        in.read(reinterpret_cast<char *>(&dataBytes), sizeof(dataBytes));
        if (!in || nameChars > 32767 || dataBytes > 16 * 1024 * 1024) {
            if (error) *error = L"Uninstall-registration recovery record is invalid.";
            return false;
        }
        std::wstring name(nameChars, L'\0');
        std::vector<BYTE> data(dataBytes);
        if (nameChars)
            in.read(reinterpret_cast<char *>(name.data()),
                    static_cast<std::streamsize>(nameChars * sizeof(wchar_t)));
        if (dataBytes)
            in.read(reinterpret_cast<char *>(data.data()),
                    static_cast<std::streamsize>(dataBytes));
        if (!in) {
            if (error) *error = L"Uninstall-registration recovery snapshot is truncated.";
            return false;
        }
        records.push_back({std::move(name), static_cast<DWORD>(type), std::move(data)});
    }
    // Reject trailing garbage so a corrupted snapshot is never partially trusted.
    char extra = 0;
    if (in.read(&extra, 1)) {
        if (error) *error = L"Uninstall-registration recovery snapshot has unexpected trailing data.";
        return false;
    }
    return true;
}

bool restoreUninstallRegistry(const std::wstring &scope,
                              const std::wstring &backup,
                              std::wstring *error)
{
    std::vector<RegistryValueSnapshot> records;
    if (!readRegistrySnapshot(backup, records, error))
        return false;

    const HKEY root = rootForScope(scope);
    if (!root) {
        if (error) *error = L"Unknown registry restore scope.";
        return false;
    }

    const wchar_t *leaf = wcsrchr(UninstallKey, L'\\');
    if (!leaf) {
        if (error) *error = L"Uninstall registry key is invalid.";
        return false;
    }
    const std::wstring parentPath(UninstallKey, static_cast<size_t>(leaf - UninstallKey));
    HKEY parent = nullptr;
    const LONG openParent = RegCreateKeyExW(
        root, parentPath.c_str(), 0, nullptr, 0,
        KEY_WRITE | viewForRoot(root), nullptr, &parent, nullptr);
    if (openParent != ERROR_SUCCESS) {
        if (error) *error = L"Could not open uninstall registry parent for recovery.";
        return false;
    }
    RegDeleteTreeW(parent, leaf + 1);
    RegCloseKey(parent);

    HKEY key = nullptr;
    const LONG create = RegCreateKeyExW(
        root, UninstallKey, 0, nullptr, 0,
        KEY_WRITE | viewForRoot(root), nullptr, &key, nullptr);
    if (create != ERROR_SUCCESS) {
        if (error) *error = L"Could not recreate uninstall registration.";
        return false;
    }
    for (const auto &record : records) {
        const LONG result = RegSetValueExW(
            key, record.name.c_str(), 0, record.type,
            record.data.empty() ? nullptr : record.data.data(),
            static_cast<DWORD>(record.data.size()));
        if (result != ERROR_SUCCESS) {
            RegCloseKey(key);
            if (error) *error = L"Could not restore an uninstall registration value.";
            return false;
        }
    }
    RegCloseKey(key);
    return true;
}

bool removeStaleRegistration(HKEY root, const std::wstring &expectedPath,
                             const std::wstring &log)
{
    std::wstring registered;
    if (!readRegisteredPath(root, registered) || !samePath(registered, expectedPath)) {
        appendLog(log, L"Stale-registration cleanup refused: registry path changed.");
        return false;
    }
    if (pathExists(registered + L"\\" + AppExeName)
        || pathExists(registered + L"\\" + UninstallerName)) {
        appendLog(log, L"Stale-registration cleanup refused: registered installation still has files.");
        return false;
    }

    const REGSAM view = root == HKEY_LOCAL_MACHINE ? KEY_WOW64_64KEY : 0;
    HKEY parent = nullptr;
    const wchar_t *leaf = wcsrchr(UninstallKey, L'\\');
    if (!leaf)
        return false;
    const std::wstring parentPath(UninstallKey, static_cast<size_t>(leaf - UninstallKey));
    if (RegOpenKeyExW(root, parentPath.c_str(), 0, KEY_WRITE | view, &parent) != ERROR_SUCCESS)
        return false;
    const LONG result = RegDeleteTreeW(parent, leaf + 1);
    RegCloseKey(parent);
    if (result != ERROR_SUCCESS) {
        appendLog(log, L"Stale-registration cleanup failed with registry error "
            + std::to_wstring(result) + L".");
        return false;
    }
    appendLog(log, L"Verified stale uninstall registration removed.");
    return true;
}

bool copyTree(const std::wstring &source, const std::wstring &destination,
              std::wstring *error)
{
    std::error_code ec;
    const fs::path src(source);
    const fs::path dst(destination);
    if (!fs::is_directory(src, ec) || ec) {
        if (error) *error = L"Source installation directory is unavailable.";
        return false;
    }
    fs::remove_all(dst, ec);
    ec.clear();
    fs::create_directories(dst.parent_path(), ec);
    if (ec) {
        if (error) *error = L"Could not create recovery parent directory.";
        return false;
    }
    fs::copy(src, dst,
             fs::copy_options::recursive
             | fs::copy_options::overwrite_existing
             | fs::copy_options::skip_symlinks,
             ec);
    if (ec || !fs::exists(dst / AppExeName)) {
        if (error) *error = L"Could not create a complete recovery snapshot.";
        return false;
    }
    return true;
}

bool restoreTree(const std::wstring &backup, const std::wstring &target,
                 std::wstring *error)
{
    std::error_code ec;
    const fs::path src(backup);
    const fs::path dst(target);
    if (!fs::is_directory(src, ec) || ec || !fs::exists(src / AppExeName)) {
        if (error) *error = L"Recovery snapshot is missing or incomplete.";
        return false;
    }
    fs::remove_all(dst, ec);
    if (ec) {
        if (error) *error = L"Could not remove the failed installation.";
        return false;
    }
    fs::create_directories(dst.parent_path(), ec);
    if (ec) {
        if (error) *error = L"Could not recreate the installation parent directory.";
        return false;
    }
    fs::copy(src, dst,
             fs::copy_options::recursive
             | fs::copy_options::overwrite_existing
             | fs::copy_options::skip_symlinks,
             ec);
    if (ec || !fs::exists(dst / AppExeName)) {
        if (error) *error = L"Could not restore the previous application files.";
        return false;
    }
    return true;
}

std::wstring currentExecutable()
{
    std::vector<wchar_t> buffer(32768);
    const DWORD len = GetModuleFileNameW(nullptr, buffer.data(),
                                         static_cast<DWORD>(buffer.size()));
    if (!len || len >= buffer.size())
        return {};
    return std::wstring(buffer.data(), len);
}

bool restoreForRequest(const Request &request, std::wstring *error)
{
    const std::wstring targetDir = parentDirectory(request.app);
    if (targetDir.empty()) {
        if (error) *error = L"Previous installation directory is invalid.";
        return false;
    }

    if (request.scope == L"user") {
        if (!restoreTree(request.backup, targetDir, error))
            return false;
        return restoreUninstallRegistry(request.scope, request.backup, error);
    }

    const std::wstring helper = currentExecutable();
    if (helper.empty()) {
        if (error) *error = L"Recovery helper path is unavailable.";
        return false;
    }
    const std::wstring params = L"--restore-backup " + quote(request.backup)
        + L" --target " + quote(targetDir)
        + L" --scope " + quote(request.scope)
        + L" --log " + quote(request.log);
    DWORD exitCode = ~0UL;
    const DWORD launch = runElevatedAndWait(helper, params, InstallTimeoutMs, exitCode);
    if (launch != ERROR_SUCCESS || exitCode != 0) {
        if (error) {
            *error = L"Elevated recovery failed (Windows error "
                + std::to_wstring(launch) + L", exit code "
                + std::to_wstring(exitCode) + L").";
        }
        return false;
    }
    return true;
}

bool healthCheck(const std::wstring &app, const std::wstring &version)
{
    DWORD healthExit = ~0UL;
    const DWORD error = runAndWait(app,
        quote(L"--update-health-check=" + version), HealthTimeoutMs, healthExit);
    return error == ERROR_SUCCESS && healthExit == 0;
}

int rollbackAndRelaunch(const Request &request, int code,
                        const std::wstring &reason)
{
    logLine(request, L"Update failed; starting executable rollback: " + reason);
    std::wstring restoreError;
    if (!restoreForRequest(request, &restoreError)) {
        return fail(request, code,
            reason + L"\n\nRollback also failed: " + restoreError
            + L"\nRecovery snapshot was kept at:\n" + request.backup);
    }

    if (!healthCheck(request.app, request.previousVersion)) {
        return fail(request, code,
            reason + L"\n\nPrevious files were restored, but the restored application "
            L"did not pass its version/startup check. Recovery snapshot was kept at:\n"
            + request.backup);
    }

    std::error_code ec;
    fs::remove_all(fs::path(request.backup), ec);
    logLine(request, L"Rollback health check passed; relaunching previous K500.");
    if (!startApplication(request.app)) {
        return fail(request, code,
            reason + L"\n\nPrevious version was restored but could not be relaunched.");
    }

    return fail(request, code,
        reason + L"\n\nThe previous SonKuPik K500 version was restored and reopened.");
}

bool cleanupPerUserMigration(const Request &request)
{
    const std::wstring targetDir = parentDirectory(request.targetApp);
    const std::wstring uninstaller = targetDir + L"\\" + UninstallerName;
    if (!pathExists(uninstaller))
        return true;
    DWORD exitCode = ~0UL;
    const DWORD error = runAndWait(uninstaller,
        L"/VERYSILENT /SUPPRESSMSGBOXES /NORESTART",
        InstallTimeoutMs, exitCode);
    return error == ERROR_SUCCESS && exitCode == 0;
}

bool restoreMachineMigrationSource(const Request &request, std::wstring *error)
{
    const std::wstring sourceDir = parentDirectory(request.app);
    const std::wstring helper = currentExecutable();
    if (sourceDir.empty() || helper.empty()) {
        if (error) *error = L"Machine migration recovery path is unavailable.";
        return false;
    }

    const std::wstring params = L"--restore-backup " + quote(request.backup)
        + L" --target " + quote(sourceDir)
        + L" --scope " + quote(L"machine")
        + L" --log " + quote(request.log);
    DWORD exitCode = ~0UL;
    const DWORD launch = runElevatedAndWait(helper, params, InstallTimeoutMs, exitCode);
    if (launch != ERROR_SUCCESS || exitCode != 0) {
        if (error) {
            *error = L"Machine migration recovery failed (Windows error "
                + std::to_wstring(launch) + L", exit code "
                + std::to_wstring(exitCode) + L").";
        }
        return false;
    }
    if (!healthCheck(request.app, request.previousVersion)) {
        if (error) *error = L"Restored machine installation failed its previous-version health check.";
        return false;
    }
    return true;
}

int migrateMachineToUser(const Request &request, HANDLE lockedSetup)
{
    const std::wstring sourceDir = parentDirectory(request.app);
    const std::wstring targetDir = parentDirectory(request.targetApp);
    if (sourceDir.empty() || targetDir.empty() || sourceDir == targetDir) {
        CloseHandle(lockedSetup);
        return fail(request, 30, L"Migration source/target installation paths are invalid.");
    }

    // Snapshot the machine installation AND its uninstall registration before
    // any migration work. The old installation remains authoritative until the
    // per-user target has passed health check and old uninstall begins.
    std::wstring backupError;
    if (!copyTree(sourceDir, request.backup, &backupError)
        || !snapshotUninstallRegistry(L"machine", request.backup, &backupError)) {
        CloseHandle(lockedSetup);
        std::error_code cleanup;
        fs::remove_all(fs::path(request.backup), cleanup);
        return fail(request, 30,
            L"Could not create the machine migration recovery snapshot: " + backupError);
    }
    logLine(request, L"Machine migration recovery snapshot created at " + request.backup);

    const auto discardBackup = [&request] {
        std::error_code ec;
        fs::remove_all(fs::path(request.backup), ec);
    };

    const std::wstring args =
        L"/VERYSILENT /SUPPRESSMSGBOXES /NORESTART /CLOSEAPPLICATIONS "
        L"/AUToupdate=1 /HELPERUPDATE=1 /MIGRATEFROMMACHINE=1 /LOG="
        + quote(request.log + L".inno.log")
        + L" /DIR=" + quote(targetDir);

    DWORD installerExit = ~0UL;
    const DWORD launchError = runAndWait(request.setup, args, InstallTimeoutMs, installerExit);
    CloseHandle(lockedSetup);

    if (launchError == ERROR_TIMEOUT) {
        // The per-user installer may still be active. Never race it with cleanup
        // or old-install recovery; preserve both old machine install and snapshot.
        return fail(request, 31,
            L"Per-user migration installer exceeded the safety timeout. The old "
            L"Program Files installation was not removed. Recovery snapshot was kept at:\n"
            + request.backup);
    }

    if (launchError != ERROR_SUCCESS || installerExit != 0) {
        cleanupPerUserMigration(request);
        discardBackup();
        if (startApplication(request.app))
            logLine(request, L"Migration failed before machine uninstall; original app reopened.");
        return fail(request, 31,
            L"Per-user migration installer did not finish successfully. "
            L"The existing Program Files installation was kept.");
    }

    if (!healthCheck(request.targetApp, request.version)) {
        cleanupPerUserMigration(request);
        discardBackup();
        if (startApplication(request.app))
            logLine(request, L"Migration target health check failed; original app reopened.");
        return fail(request, 32,
            L"The per-user installation failed its startup/version check. "
            L"It was removed and the existing Program Files installation was kept.");
    }

    const std::wstring oldUninstaller = sourceDir + L"\\" + UninstallerName;
    if (!pathExists(oldUninstaller)) {
        cleanupPerUserMigration(request);
        discardBackup();
        startApplication(request.app);
        return fail(request, 33,
            L"The Program Files uninstaller is missing. Migration was rolled back "
            L"before removing the existing installation.");
    }

    DWORD uninstallExit = ~0UL;
    const DWORD uninstallError = runElevatedAndWait(oldUninstaller,
        L"/VERYSILENT /SUPPRESSMSGBOXES /NORESTART",
        InstallTimeoutMs, uninstallExit);

    if (uninstallError == ERROR_CANCELLED) {
        // UAC was declined before the old uninstaller started. The machine
        // installation is still authoritative, so remove only the new user copy.
        const bool cleaned = cleanupPerUserMigration(request);
        discardBackup();
        startApplication(request.app);
        return fail(request, 34,
            L"Administrator permission to remove the old Program Files installation "
            L"was cancelled. The old application was kept and reopened."
            + std::wstring(cleaned
                ? L" The temporary per-user installation was removed."
                : L" The temporary per-user cleanup failed; inspect the update log."));
    }

    if (uninstallError == ERROR_TIMEOUT) {
        // Never race a possibly-running elevated uninstaller. Keep the healthy
        // per-user copy and recovery snapshot untouched for deterministic repair.
        return fail(request, 34,
            L"The old Program Files uninstaller exceeded the safety timeout. It may "
            L"still be running, so SonKuPik did not restore or delete either installation. "
            L"Recovery snapshot was kept at:\n" + request.backup);
    }

    if (uninstallError != ERROR_SUCCESS || uninstallExit != 0
        || pathExists(request.app)) {
        const bool cleaned = cleanupPerUserMigration(request);
        std::wstring restoreError;
        const bool restored = restoreMachineMigrationSource(request, &restoreError);
        if (restored) {
            discardBackup();
            startApplication(request.app);
            return fail(request, 34,
                L"Windows could not complete removal of the old Program Files installation. "
                L"The machine installation and uninstall registration were restored and reopened."
                + std::wstring(cleaned
                    ? L" The temporary per-user installation was removed."
                    : L" The temporary per-user cleanup also failed; inspect the update log."));
        }
        return fail(request, 34,
            L"Windows could not complete removal of the old Program Files installation. "
            L"Automatic recovery also failed: " + restoreError
            + L"\nRecovery snapshot was kept at:\n" + request.backup);
    }

    discardBackup();
    logLine(request, L"Machine->user migration verified; old installation removed and recovery snapshot released.");
    if (!startApplication(request.targetApp))
        return fail(request, 35,
            L"Migration completed and the new installation passed validation, "
            L"but K500 could not be relaunched.");
    return 0;
}

int selfTest()
{
    if (quote(L"C:\\Some Folder\\app.exe") != L"\"C:\\Some Folder\\app.exe\"") return 31;
    if (quote(L"C:\\trailing\\") != L"\"C:\\trailing\\\\\"") return 32;
    if (quote(L"has\"quote") != L"\"has\\\"quote\"") return 33;
    if (parentDirectory(L"C:\\Program Files\\SonKuPik K500\\SonKuPik-K500.exe")
        != L"C:\\Program Files\\SonKuPik K500") return 34;
    if (!parentDirectory(L"SonKuPik-K500.exe").empty()) return 35;

    wchar_t tempPath[MAX_PATH]{};
    if (!GetTempPathW(MAX_PATH, tempPath)) return 36;
    const fs::path root = fs::path(tempPath) / L"SonKuPik-K500-helper-selftest";
    std::error_code ec;
    fs::remove_all(root, ec);
    fs::create_directories(root / L"source", ec);
    if (ec) return 37;
    {
        HANDLE file = CreateFileW((root / L"source" / AppExeName).c_str(),
                                  GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                                  FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) return 38;
        const char data[] = "old";
        DWORD written = 0;
        WriteFile(file, data, sizeof(data), &written, nullptr);
        CloseHandle(file);
    }
    std::wstring error;
    if (!copyTree((root / L"source").wstring(), (root / L"backup").wstring(), &error))
        return 39;
    fs::remove_all(root / L"source", ec);
    if (!restoreTree((root / L"backup").wstring(), (root / L"source").wstring(), &error))
        return 40;
    if (!fs::exists(root / L"source" / AppExeName))
        return 41;
    fs::remove_all(root, ec);
    return 0;
}
} // namespace

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    int argc = 0;
    wchar_t **argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv)
        return 10;

    if (argc == 2 && std::wstring(argv[1]) == L"--self-test") {
        const int result = selfTest();
        LocalFree(argv);
        return result;
    }

    if (argc == 4 && std::wstring(argv[1]) == L"--verify-self-test") {
        Request fixture;
        fixture.setup = argv[2];
        fixture.sha256 = argv[3];
        LocalFree(argv);
        HANDLE locked = INVALID_HANDLE_VALUE;
        const bool ok = verifySetup(fixture, locked);
        if (locked != INVALID_HANDLE_VALUE)
            CloseHandle(locked);
        return ok ? 0 : 41;
    }

    if (argc == 7 && std::wstring(argv[1]) == L"--remove-stale-registration"
        && std::wstring(argv[3]) == L"--expected-path"
        && std::wstring(argv[5]) == L"--log") {
        const std::wstring scope = argv[2];
        const std::wstring expected = argv[4];
        const std::wstring log = argv[6];
        LocalFree(argv);
        const HKEY root = scope == L"machine" ? HKEY_LOCAL_MACHINE
            : scope == L"user" ? HKEY_CURRENT_USER : nullptr;
        if (!root)
            return 62;
        return removeStaleRegistration(root, expected, log) ? 0 : 63;
    }

    if (argc == 9 && std::wstring(argv[1]) == L"--restore-backup"
        && std::wstring(argv[3]) == L"--target"
        && std::wstring(argv[5]) == L"--scope"
        && std::wstring(argv[7]) == L"--log") {
        const std::wstring backup = argv[2];
        const std::wstring target = argv[4];
        const std::wstring scope = argv[6];
        const std::wstring log = argv[8];
        LocalFree(argv);
        std::wstring error;
        if (!restoreTree(backup, target, &error)
            || !restoreUninstallRegistry(scope, backup, &error)) {
            appendLog(log, L"Elevated restore failed: " + error);
            return 61;
        }
        appendLog(log, L"Elevated application + uninstall-registration recovery snapshot restored.");
        return 0;
    }

    Request request;
    const bool valid = parse(argc, argv, request);
    LocalFree(argv);
    if (!valid)
        return 12;

    logLine(request, L"Starting verified P3 update handoff.");
    if (!waitParent(request))
        return fail(request, 13, L"K500 did not exit safely. Installation was not started.");

    HANDLE lockedSetup = INVALID_HANDLE_VALUE;
    if (!verifySetup(request, lockedSetup))
        return fail(request, 14, L"Installer SHA-256 changed after download. Nothing was installed.");

    if (request.mode == L"migrate")
        return migrateMachineToUser(request, lockedSetup);

    const std::wstring installDir = parentDirectory(request.app);
    if (installDir.empty()) {
        CloseHandle(lockedSetup);
        return fail(request, 19, L"Installed application path has no valid parent directory.");
    }

    std::wstring backupError;
    if (!copyTree(installDir, request.backup, &backupError)) {
        CloseHandle(lockedSetup);
        return fail(request, 20,
            L"Could not create a recovery snapshot before installation: " + backupError);
    }
    if (!snapshotUninstallRegistry(request.scope, request.backup, &backupError)) {
        CloseHandle(lockedSetup);
        std::error_code cleanup;
        fs::remove_all(fs::path(request.backup), cleanup);
        return fail(request, 20,
            L"Could not snapshot the uninstall registration before installation: " + backupError);
    }
    logLine(request, L"Application files and uninstall registration snapshot created at " + request.backup);

    const std::wstring args =
        L"/VERYSILENT /SUPPRESSMSGBOXES /NORESTART /CLOSEAPPLICATIONS "
        L"/AUToupdate=1 /HELPERUPDATE=1 /LOG=" + quote(request.log + L".inno.log")
        + L" /DIR=" + quote(installDir);

    DWORD installerExit = ~0UL;
    const DWORD launchError = request.scope == L"user"
        ? runAndWait(request.setup, args, InstallTimeoutMs, installerExit)
        : runElevatedAndWait(request.setup, args, InstallTimeoutMs, installerExit);
    CloseHandle(lockedSetup);

    if (launchError == ERROR_CANCELLED) {
        std::error_code ec;
        fs::remove_all(fs::path(request.backup), ec);
        logLine(request, L"Administrator prompt was cancelled before installation.");
        if (!startApplication(request.app))
            return fail(request, 15,
                L"Administrator permission was cancelled; reopen K500 manually.");
        return 15;
    }

    if (launchError == ERROR_TIMEOUT) {
        return fail(request, 21,
            L"The installer did not finish within the safety timeout. It was not terminated "
            L"or rolled back because it may still be running. Recovery snapshot was kept at:\n"
            + request.backup);
    }

    if (launchError != ERROR_SUCCESS) {
        return rollbackAndRelaunch(request, 22,
            L"Windows could not complete the installer process (error "
            + std::to_wstring(launchError) + L").");
    }

    if (installerExit != 0) {
        return rollbackAndRelaunch(request, 23,
            L"Installer exited with code " + std::to_wstring(installerExit) + L".");
    }

    if (!healthCheck(request.targetApp, request.version)) {
        return rollbackAndRelaunch(request, 24,
            L"Installed K500 did not pass the version/startup health check.");
    }

    std::error_code ec;
    fs::remove_all(fs::path(request.backup), ec);
    logLine(request, L"Installer and app health check passed; recovery snapshot released.");
    if (!startApplication(request.targetApp))
        return fail(request, 18,
            L"Updated K500 passed validation but could not be launched.");
    return 0;
}
