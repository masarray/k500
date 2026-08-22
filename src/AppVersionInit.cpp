#include <QCoreApplication>
#include <QString>

#ifndef SONKUPIK_VERSION
#define SONKUPIK_VERSION "0.0.0-dev"
#endif

namespace {
void initializeSonkupikVersion()
{
    // P5_RUNTIME_VERSION_V1 — single source of truth is CMake project VERSION.
    QCoreApplication::setApplicationVersion(QStringLiteral(SONKUPIK_VERSION));
}
}

Q_COREAPP_STARTUP_FUNCTION(initializeSonkupikVersion)
