#include "RuntimeMetrics.h"

#ifdef Q_OS_WIN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#endif

RuntimeHealthSnapshot RuntimeMetrics::capture()
{
    RuntimeHealthSnapshot snapshot;

#ifdef Q_OS_WIN
    PROCESS_MEMORY_COUNTERS_EX memory{};
    memory.cb = sizeof(memory);
    const bool memoryOk = GetProcessMemoryInfo(
        GetCurrentProcess(),
        reinterpret_cast<PROCESS_MEMORY_COUNTERS *>(&memory),
        sizeof(memory));

    quint32 threadCount = 0;
    const DWORD processId = GetCurrentProcessId();
    HANDLE threadSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    bool threadsOk = false;
    if (threadSnapshot != INVALID_HANDLE_VALUE) {
        THREADENTRY32 entry{};
        entry.dwSize = sizeof(entry);
        if (Thread32First(threadSnapshot, &entry)) {
            threadsOk = true;
            do {
                if (entry.th32OwnerProcessID == processId)
                    ++threadCount;
            } while (Thread32Next(threadSnapshot, &entry));
        }
        CloseHandle(threadSnapshot);
    }

    DWORD handles = 0;
    const bool handlesOk = GetProcessHandleCount(GetCurrentProcess(), &handles);

    if (memoryOk) {
        snapshot.workingSetBytes = static_cast<qint64>(memory.WorkingSetSize);
        snapshot.privateBytes = static_cast<qint64>(memory.PrivateUsage);
        snapshot.peakWorkingSetBytes = static_cast<qint64>(memory.PeakWorkingSetSize);
    }
    if (handlesOk)
        snapshot.handleCount = handles;
    if (threadsOk)
        snapshot.threadCount = threadCount;

    snapshot.valid = memoryOk && handlesOk && threadsOk;
#else
    // Native K500 runtime qualification is Windows-only. Keep a safe fallback
    // so the helper remains buildable on other hosts without inventing metrics.
    snapshot.valid = false;
#endif

    return snapshot;
}
