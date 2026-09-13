#ifdef _WIN32

#include <windows.h>
#include <shellapi.h>
#include <bcrypt.h>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "shell32.lib")

namespace {

struct BcryptAlgorithm final {
    BCRYPT_ALG_HANDLE handle = nullptr;
    ~BcryptAlgorithm() { if (handle) BCryptCloseAlgorithmProvider(handle, 0); }
};

struct BcryptHash final {
    BCRYPT_HASH_HANDLE handle = nullptr;
    ~BcryptHash() { if (handle) BCryptDestroyHash(handle); }
};

struct WinHandle final {
    HANDLE handle = nullptr;
    ~WinHandle() { if (handle && handle != INVALID_HANDLE_VALUE) CloseHandle(handle); }
};

std::wstring argumentValue(int argc, wchar_t **argv, const std::wstring &key)
{
    for (int i = 1; i + 1 < argc; ++i) {
        if (key == argv[i])
            return argv[i + 1];
    }
    return {};
}

std::wstring quoteArg(const std::wstring &value)
{
    std::wstring escaped = value;
    size_t pos = 0;
    while ((pos = escaped.find(L'"', pos)) != std::wstring::npos) {
        escaped.insert(pos, L"\\");
        pos += 2;
    }
    return L"\"" + escaped + L"\"";
}

bool waitForProcess(DWORD pid, DWORD timeoutMs)
{
    if (pid == 0)
        return true;
    WinHandle process{OpenProcess(SYNCHRONIZE, FALSE, pid)};
    if (!process.handle)
        return true; // already exited
    return WaitForSingleObject(process.handle, timeoutMs) == WAIT_OBJECT_0;
}

std::wstring sha256File(const std::filesystem::path &path)
{
    BcryptAlgorithm algorithm;
    if (BCryptOpenAlgorithmProvider(&algorithm.handle, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0)
        return {};

    DWORD objectLength = 0;
    DWORD cbResult = 0;
    if (BCryptGetProperty(algorithm.handle, BCRYPT_OBJECT_LENGTH,
                          reinterpret_cast<PUCHAR>(&objectLength), sizeof(objectLength),
                          &cbResult, 0) < 0) {
        return {};
    }

    DWORD hashLength = 0;
    if (BCryptGetProperty(algorithm.handle, BCRYPT_HASH_LENGTH,
                          reinterpret_cast<PUCHAR>(&hashLength), sizeof(hashLength),
                          &cbResult, 0) < 0) {
        return {};
    }

    std::vector<unsigned char> hashObject(objectLength);
    std::vector<unsigned char> digest(hashLength);
    BcryptHash hash;
    if (BCryptCreateHash(algorithm.handle, &hash.handle,
                         hashObject.data(), static_cast<ULONG>(hashObject.size()),
                         nullptr, 0, 0) < 0) {
        return {};
    }

    std::ifstream stream(path, std::ios::binary);
    if (!stream)
        return {};

    std::vector<unsigned char> buffer(1024 * 1024);
    while (stream) {
        stream.read(reinterpret_cast<char *>(buffer.data()), static_cast<std::streamsize>(buffer.size()));
        const std::streamsize count = stream.gcount();
        if (count > 0 && BCryptHashData(hash.handle, buffer.data(), static_cast<ULONG>(count), 0) < 0)
            return {};
    }

    if (BCryptFinishHash(hash.handle, digest.data(), static_cast<ULONG>(digest.size()), 0) < 0)
        return {};

    std::wostringstream out;
    out << std::hex << std::setfill(L'0');
    for (unsigned char byte : digest)
        out << std::setw(2) << static_cast<unsigned int>(byte);
    return out.str();
}

void launchApp(const std::filesystem::path &appPath)
{
    if (appPath.empty() || !std::filesystem::exists(appPath))
        return;
    ShellExecuteW(nullptr, L"open", appPath.c_str(), nullptr,
                  appPath.parent_path().c_str(), SW_SHOWNORMAL);
}

int runElevatedInstaller(const std::filesystem::path &installer)
{
    const std::wstring parameters =
        L"/VERYSILENT /SUPPRESSMSGBOXES /NORESTART /CLOSEAPPLICATIONS /AUTOUPDATE=1";

    SHELLEXECUTEINFOW info{};
    info.cbSize = sizeof(info);
    info.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
    info.lpVerb = L"runas";
    info.lpFile = installer.c_str();
    info.lpParameters = parameters.c_str();
    info.lpDirectory = installer.parent_path().c_str();
    info.nShow = SW_SHOWNORMAL;

    if (!ShellExecuteExW(&info))
        return static_cast<int>(GetLastError() ? GetLastError() : ERROR_CANCELLED);

    WinHandle process{info.hProcess};
    if (!process.handle)
        return ERROR_INVALID_HANDLE;

    WaitForSingleObject(process.handle, INFINITE);
    DWORD exitCode = ERROR_GEN_FAILURE;
    if (!GetExitCodeProcess(process.handle, &exitCode))
        return static_cast<int>(GetLastError());
    return static_cast<int>(exitCode);
}

} // namespace

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    int argc = 0;
    wchar_t **argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argv)
        return 2;

    const std::filesystem::path installer(argumentValue(argc, argv, L"--installer"));
    const std::filesystem::path appPath(argumentValue(argc, argv, L"--app"));
    std::wstring expectedHash = argumentValue(argc, argv, L"--sha256");
    const std::wstring expectedBytesText = argumentValue(argc, argv, L"--bytes");
    const std::wstring pidText = argumentValue(argc, argv, L"--pid");
    LocalFree(argv);

    std::transform(expectedHash.begin(), expectedHash.end(), expectedHash.begin(), ::towlower);

    std::uintmax_t expectedBytes = 0;
    DWORD pid = 0;
    try {
        expectedBytes = expectedBytesText.empty() ? 0 : std::stoull(expectedBytesText);
        pid = pidText.empty() ? 0 : static_cast<DWORD>(std::stoul(pidText));
    } catch (...) {
        launchApp(appPath);
        return 3;
    }

    // Wait for the GUI to fully exit before the elevated Inno process starts.
    // This guarantees AppMutex/file locks are released rather than racing the
    // installer and is the key to a reliable self-update handoff.
    if (!waitForProcess(pid, 60000)) {
        launchApp(appPath);
        return 4;
    }

    if (installer.empty() || appPath.empty() || expectedHash.size() != 64
        || !std::filesystem::exists(installer)) {
        launchApp(appPath);
        return 5;
    }

    std::error_code ec;
    const std::uintmax_t actualBytes = std::filesystem::file_size(installer, ec);
    if (ec || (expectedBytes > 0 && actualBytes != expectedBytes)) {
        launchApp(appPath);
        return 6;
    }

    // Defense in depth: re-hash after the main process exits so a local race
    // cannot replace the already-verified installer between verification and UAC.
    const std::wstring actualHash = sha256File(installer);
    if (actualHash.empty() || actualHash != expectedHash) {
        launchApp(appPath);
        return 7;
    }

    const int installExit = runElevatedInstaller(installer);
    if (installExit == 0) {
        std::filesystem::remove(installer, ec); // best effort cache cleanup
        launchApp(appPath);
        return 0;
    }

    // UAC cancel or installer failure must not strand a novice user. Relaunch
    // the existing version; Program Files is only replaced after Inno succeeds.
    launchApp(appPath);
    return installExit == 0 ? 1 : installExit;
}

#else
int main() { return 1; }
#endif
