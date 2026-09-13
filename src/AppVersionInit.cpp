#include "AppUpdateManager.h"

#include <QCoreApplication>
#include <QDir>
#include <QQuickWindow>
#include <QSettings>
#include <QStandardPaths>
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

    // USER_PRESET_LIBRARY_V1 — program binaries live in Program Files while
    // user-authored .k500 data gets a visible, backup-friendly Documents home.
    // Explicit org/app keys are required here because this startup hook executes
    // before main() assigns QCoreApplication organization/application names.
    QSettings settings(QStringLiteral("MasArray"), QStringLiteral("SonKuPik K500"));
    const QString presetKey = QStringLiteral("pcPresetLibrary/folder");
    const QString rememberedFolder = QDir::cleanPath(settings.value(presetKey).toString());
    if (rememberedFolder.isEmpty() || !QDir(rememberedFolder).exists()) {
        QString documents = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
        if (documents.isEmpty())
            documents = QDir::homePath() + QStringLiteral("/Documents");
        const QString defaultPresetFolder = QDir(documents).filePath(
            QStringLiteral("SonKuPik K500/Presets"));
        if (QDir().mkpath(defaultPresetFolder))
            settings.setValue(presetKey, QDir::cleanPath(defaultPresetFolder));
    }

    // UI_NATIVE_TEXT_RENDERING_V1
    // SonKuPik is a dense Windows desktop control surface with many 9–11 px
    // labels. Native glyph rasterization keeps those labels crisp and hinted
    // without enabling global MSAA or adding a continuously-rendered effect.
    // This must run before the first QQuickWindow is created.
    QQuickWindow::setTextRenderType(QQuickWindow::NativeTextRendering);

    // SMART_UPDATE_RUNTIME_V1 — one app-owned updater instance is available to
    // QML without exposing network/process primitives to the visual layer.
    // Register it into the existing application module so the compiled QML does
    // not depend on a second runtime-only import.
    auto *updater = new AppUpdateManager(QCoreApplication::instance());
    qmlRegisterSingletonInstance("SonkupikStudio", 1, 0, "AppUpdater", updater);
}
}

Q_COREAPP_STARTUP_FUNCTION(initializeSonkupikVersion)
