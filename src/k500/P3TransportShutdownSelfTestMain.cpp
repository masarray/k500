#include "K500WinIo.h"
#include "RuntimeMetrics.h"

#include <QCoreApplication>
#include <QDebug>
#include <QThread>

namespace {
constexpr int WarmupCycles = 8;
constexpr int PhaseCycles = 64;
constexpr qint64 MaxPrivateGrowthBytes = 4LL * 1024LL * 1024LL;
constexpr qint64 MaxHandleGrowth = 1;
constexpr qint64 MaxThreadGrowth = 0;

bool runCycles(int count, QString *error)
{
    for (int i = 0; i < count; ++i) {
        K500WinIo io;

        // P3_DETERMINISTIC_SHUTDOWN_V1
        // Admission closes before worker teardown. Repeated shutdown is a no-op,
        // and writes must remain rejected after the worker has been joined.
        io.close();
        io.close();
        io.shutdown();
        io.shutdown();

        if (io.isOpen() || io.kind() != K500WinIo::Kind::None || !io.label().isEmpty()) {
            if (error)
                *error = QStringLiteral("Shutdown transport facade retained open state at cycle %1").arg(i);
            return false;
        }

        QString writeError;
        const bool accepted = io.writeProtocolFrame(QByteArray::fromHex("AA031C0003E5"), &writeError);
        if (accepted || writeError.isEmpty()) {
            if (error)
                *error = QStringLiteral("Shutdown transport accepted a frame or omitted error at cycle %1").arg(i);
            return false;
        }

        QString openError;
        if (io.openSerial(QStringLiteral("COM65535"), &openError) || openError.isEmpty()) {
            if (error)
                *error = QStringLiteral("Shutdown transport restarted or omitted open error at cycle %1").arg(i);
            return false;
        }
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
    qInfo() << "P3 deterministic shutdown self-test is Windows-only";
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
        qCritical() << "Unable to capture P3 shutdown phase-A metrics";
        return 4;
    }

    if (!runCycles(PhaseCycles, &error)) {
        qCritical().noquote() << error;
        return 5;
    }
    const RuntimeHealthSnapshot phaseB = RuntimeMetrics::capture();
    if (!phaseB.valid) {
        qCritical() << "Unable to capture P3 shutdown phase-B metrics";
        return 6;
    }

    const qint64 privateGrowth = phaseB.privateBytes - phaseA.privateBytes;
    const qint64 handleGrowth = delta(phaseB.handleCount, phaseA.handleCount);
    const qint64 threadGrowth = delta(phaseB.threadCount, phaseA.threadCount);

    qInfo().noquote()
        << QStringLiteral("P3 deterministic shutdown: privateDelta=%1 handleDelta=%2 threadDelta=%3")
               .arg(privateGrowth).arg(handleGrowth).arg(threadGrowth);

    if (privateGrowth > MaxPrivateGrowthBytes) {
        qCritical() << "P3 shutdown private-memory growth exceeded guard:" << privateGrowth;
        return 7;
    }
    if (handleGrowth > MaxHandleGrowth) {
        qCritical() << "P3 shutdown handle growth exceeded guard:" << handleGrowth;
        return 8;
    }
    if (threadGrowth > MaxThreadGrowth) {
        qCritical() << "P3 shutdown thread growth exceeded guard:" << threadGrowth;
        return 9;
    }

    qInfo() << "P3 deterministic shutdown self-test passed";
    return 0;
#endif
}
