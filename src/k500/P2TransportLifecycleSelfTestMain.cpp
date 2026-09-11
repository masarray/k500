#include "K500WinIo.h"
#include "RuntimeMetrics.h"

#include <QCoreApplication>
#include <QDebug>
#include <QThread>

namespace {
constexpr int WarmupCycles = 8;
constexpr int PhaseCycles = 48;
constexpr qint64 MaxPrivateGrowthBytes = 4LL * 1024LL * 1024LL;
constexpr qint64 MaxHandleGrowth = 1;
constexpr qint64 MaxThreadGrowth = 0;

bool runCycles(int count, QString *error)
{
    for (int i = 0; i < count; ++i) {
        K500WinIo io;
        if (io.isOpen() || io.kind() != K500WinIo::Kind::None || !io.label().isEmpty()) {
            if (error)
                *error = QStringLiteral("Fresh transport facade is unexpectedly open at cycle %1").arg(i);
            return false;
        }

        QString writeError;
        const bool accepted = io.writeProtocolFrame(QByteArray::fromHex("AA031C0003E5"), &writeError);
        if (accepted || writeError.isEmpty()) {
            if (error)
                *error = QStringLiteral("Closed transport accepted a frame or failed to report an error at cycle %1").arg(i);
            return false;
        }

        // P2_TRANSPORT_LIFECYCLE_STRESS_V1
        // close() is intentionally exercised repeatedly before destruction.
        // The public call must be idempotent, and destruction must synchronously
        // join the worker without leaving a thread or native handle behind.
        io.close();
        io.close();
    }

    QCoreApplication::processEvents();
    QThread::msleep(10);
    QCoreApplication::processEvents();
    return true;
}

qint64 delta(quint32 after, quint32 before)
{
    return static_cast<qint64>(after) - static_cast<qint64>(before);
}
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

#ifndef Q_OS_WIN
    qInfo() << "P2 transport lifecycle self-test is Windows-only";
    return 0;
#else
    QString error;
    if (!runCycles(WarmupCycles, &error)) {
        qCritical().noquote() << error;
        return 2;
    }

    if (!runCycles(PhaseCycles, &error)) {
        qCritical().noquote() << error;
        return 3;
    }
    const RuntimeHealthSnapshot phaseA = RuntimeMetrics::capture();
    if (!phaseA.valid) {
        qCritical() << "Unable to capture P2 phase-A runtime metrics";
        return 4;
    }

    if (!runCycles(PhaseCycles, &error)) {
        qCritical().noquote() << error;
        return 5;
    }
    const RuntimeHealthSnapshot phaseB = RuntimeMetrics::capture();
    if (!phaseB.valid) {
        qCritical() << "Unable to capture P2 phase-B runtime metrics";
        return 6;
    }

    const qint64 privateGrowth = phaseB.privateBytes - phaseA.privateBytes;
    const qint64 handleGrowth = delta(phaseB.handleCount, phaseA.handleCount);
    const qint64 threadGrowth = delta(phaseB.threadCount, phaseA.threadCount);

    qInfo().noquote()
        << QStringLiteral("P2 transport lifecycle: privateDelta=%1 handleDelta=%2 threadDelta=%3")
               .arg(privateGrowth).arg(handleGrowth).arg(threadGrowth);

    if (privateGrowth > MaxPrivateGrowthBytes) {
        qCritical() << "P2 lifecycle private-memory growth exceeded guard:" << privateGrowth;
        return 7;
    }
    if (handleGrowth > MaxHandleGrowth) {
        qCritical() << "P2 lifecycle handle growth exceeded guard:" << handleGrowth;
        return 8;
    }
    if (threadGrowth > MaxThreadGrowth) {
        qCritical() << "P2 lifecycle thread growth exceeded guard:" << threadGrowth;
        return 9;
    }

    qInfo() << "P2 transport lifecycle self-test passed";
    return 0;
#endif
}
