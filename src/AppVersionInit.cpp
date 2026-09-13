#include <QCoreApplication>
#include <QQuickWindow>
#include <QString>

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
}
}

Q_COREAPP_STARTUP_FUNCTION(initializeSonkupikVersion)
