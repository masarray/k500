#include "WinResourceGuards.h"
#include "RuntimeMetrics.h"

#include <QCoreApplication>
#include <QDebug>

#ifdef Q_OS_WIN
#include <hidsdi.h>
#endif

namespace {
#ifdef Q_OS_WIN
bool exerciseResourceGuards(int cycles, QString *error)
{
    for (int i = 0; i < cycles; ++i) {
        UniqueWinHandle event(CreateEventW(nullptr, TRUE, FALSE, nullptr));
        if (!event) {
            if (error)
                *error = QStringLiteral("CreateEventW failed at cycle %1 (%2)").arg(i).arg(GetLastError());
            return false;
        }

        UniqueWinHandle moved(std::move(event));
        if (event || !moved) {
            if (error)
                *error = QStringLiteral("UniqueWinHandle move contract failed at cycle %1").arg(i);
            return false;
        }

        UniqueLocalBuffer local(LocalAlloc(LMEM_FIXED, 256));
        if (!local) {
            if (error)
                *error = QStringLiteral("LocalAlloc failed at cycle %1").arg(i);
            return false;
        }

        GUID hidGuid{};
        HidD_GetHidGuid(&hidGuid);
        UniqueDeviceInfoSet devices(
            SetupDiGetClassDevsW(&hidGuid, nullptr, nullptr,
                                 DIGCF_PRESENT | DIGCF_DEVICEINTERFACE));
        if (!devices) {
            if (error)
                *error = QStringLiteral("SetupDiGetClassDevsW failed at cycle %1 (%2)")
                             .arg(i).arg(GetLastError());
            return false;
        }

        // Explicit reset is exercised as well as destructor cleanup. Repeating
        // reset must remain harmless and never double-close the underlying OS
        // resource.
        moved.reset();
        moved.reset();
        local.reset();
        local.reset();
        devices.reset();
        devices.reset();
    }
    return true;
}
#endif

qint64 delta(quint32 after, quint32 before)
{
    return static_cast<qint64>(after) - static_cast<qint64>(before);
}
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

#ifndef Q_OS_WIN
    qInfo() << "P3 Windows RAII self-test skipped on non-Windows platform";
    return 0;
#else
    QString error;
    if (!exerciseResourceGuards(16, &error)) {
        qCritical().noquote() << error;
        return 2;
    }

    if (!exerciseResourceGuards(96, &error)) {
        qCritical().noquote() << error;
        return 3;
    }
    const RuntimeHealthSnapshot phaseA = RuntimeMetrics::capture();
    if (!phaseA.valid)
        return 4;

    if (!exerciseResourceGuards(96, &error)) {
        qCritical().noquote() << error;
        return 5;
    }
    const RuntimeHealthSnapshot phaseB = RuntimeMetrics::capture();
    if (!phaseB.valid)
        return 6;

    const qint64 handleGrowth = delta(phaseB.handleCount, phaseA.handleCount);
    const qint64 threadGrowth = delta(phaseB.threadCount, phaseA.threadCount);
    const qint64 privateGrowth = phaseB.privateBytes - phaseA.privateBytes;

    qInfo().noquote()
        << QStringLiteral("P3 RAII stress: privateDelta=%1 handleDelta=%2 threadDelta=%3")
               .arg(privateGrowth).arg(handleGrowth).arg(threadGrowth);

    if (handleGrowth > 0) {
        qCritical() << "P3 RAII handle growth detected:" << handleGrowth;
        return 7;
    }
    if (threadGrowth > 0) {
        qCritical() << "P3 RAII unexpected thread growth detected:" << threadGrowth;
        return 8;
    }
    if (privateGrowth > 2LL * 1024LL * 1024LL) {
        qCritical() << "P3 RAII private-memory growth exceeded guard:" << privateGrowth;
        return 9;
    }

    qInfo() << "P3 Windows RAII resource self-test passed";
    return 0;
#endif
}
