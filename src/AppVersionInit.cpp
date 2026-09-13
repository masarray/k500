#include "AppUpdateManager.h"

#include <QCoreApplication>
#include <QQuickWindow>
#include <QString>
#include <QtQml/qqml.h>

#ifndef SONKUPIK_VERSION
#define SONKUPIK_VERSION "0.0.0-dev"
#endif

namespace {
void initializeSonkupikVersion()
{
    // P5_RUNTIME_VERSION_V1 — single source of truth is CMake project VERSION.
    QCoreApplication::setApplicationVersion(QStringLiteral(SONKUPIK_VERSION));

    // UI_NATIVE_TEXT_RENDERING_V1
    // SonKuPik is a dense Windows desktop control surface with many 9–11 px
    // labels. Native glyph rasterization keeps those labels crisp and hinted
    // without enabling global MSAA or adding a continuously-rendered effect.
    // This must run before the first QQuickWindow is created.
    QQuickWindow::setTextRenderType(QQuickWindow::NativeTextRendering);

    // SMART_UPDATE_RUNTIME_V1 — one app-owned updater instance is available to
    // QML without exposing network/process primitives to the visual layer.
    // The updater accepts only public stable GitHub releases and verifies the
    // release manifest plus SHA-256 before Windows is allowed to execute Setup.
    auto *updater = new AppUpdateManager(QCoreApplication::instance());
    qmlRegisterSingletonInstance("SonkupikRuntime", 1, 0, "AppUpdater", updater);
}
}

Q_COREAPP_STARTUP_FUNCTION(initializeSonkupikVersion)
