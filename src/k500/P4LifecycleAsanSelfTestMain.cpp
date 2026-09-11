#include "K500WinIo.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QDebug>
#include <QString>

namespace {
constexpr int LifecycleCycles = 384;

bool runLifecycleSoak(QString *error)
{
    for (int i = 0; i < LifecycleCycles; ++i) {
        K500WinIo io;

        // P4_LIFECYCLE_ASAN_V1
        // Exercise permanent shutdown repeatedly under ASan. This is deliberately
        // hardware-free: the invalid COM path forces worker-side open failure,
        // then teardown must remain safe and idempotent.
        QString openError;
        if (io.openSerial(QStringLiteral("COM65535"), &openError)) {
            if (error) *error = QStringLiteral("unexpected serial open success at cycle %1").arg(i);
            return false;
        }
        if (openError.isEmpty()) {
            if (error) *error = QStringLiteral("serial open failure omitted error at cycle %1").arg(i);
            return false;
        }

        io.close();
        io.close();
        io.shutdown();
        io.shutdown();

        QString writeError;
        const bool accepted = io.writeProtocolFrame(QByteArray::fromHex("AA031C0003E5"), &writeError);
        if (accepted || writeError.isEmpty()) {
            if (error) *error = QStringLiteral("post-shutdown write admission regression at cycle %1").arg(i);
            return false;
        }
        if (io.isOpen() || io.kind() != K500WinIo::Kind::None || !io.label().isEmpty()) {
            if (error) *error = QStringLiteral("transport retained facade state at cycle %1").arg(i);
            return false;
        }

        if ((i & 0x1f) == 0)
            QCoreApplication::processEvents();
    }
    QCoreApplication::processEvents();
    return true;
}
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

#ifndef Q_OS_WIN
    qInfo() << "P4 lifecycle ASan soak is Windows-only";
    return 0;
#else
    QString error;
    if (!runLifecycleSoak(&error)) {
        qCritical().noquote() << "P4 lifecycle ASan soak failed:" << error;
        return 2;
    }
    qInfo() << "P4 lifecycle ASan soak passed" << LifecycleCycles << "cycles";
    return 0;
#endif
}
