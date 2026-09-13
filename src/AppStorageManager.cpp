#include "AppStorageManager.h"

#include <QDir>
#include <QMetaObject>
#include <QSettings>
#include <QStandardPaths>

namespace {
constexpr auto PresetFolderKey = "pcPresetLibrary/folder";

QString documentsBase()
{
    QString base = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (base.isEmpty())
        base = QDir::home().filePath(QStringLiteral("Documents"));
    return QDir::cleanPath(base);
}
}

AppStorageManager::AppStorageManager(QObject *parent)
    : QObject(parent)
{
    const QDir documents(documentsBase());
    m_documentsRoot = QDir::cleanPath(documents.filePath(QStringLiteral("SonKuPik K500")));
    const QDir root(m_documentsRoot);
    m_presetFolder = QDir::cleanPath(root.filePath(QStringLiteral("Presets")));
    m_backupFolder = QDir::cleanPath(root.filePath(QStringLiteral("Backups")));
    m_exportFolder = QDir::cleanPath(root.filePath(QStringLiteral("Exports")));
}

bool AppStorageManager::ensureUserStorage(QObject *presetFileBridge)
{
    if (!QDir().mkpath(m_documentsRoot)
        || !QDir().mkpath(m_presetFolder)
        || !QDir().mkpath(m_backupFolder)
        || !QDir().mkpath(m_exportFolder)) {
        return false;
    }

    if (!presetFileBridge)
        return true;

    QSettings settings;
    const bool hasRememberedFolder = settings.contains(QString::fromLatin1(PresetFolderKey));
    const QString rememberedFolder = settings.value(QString::fromLatin1(PresetFolderKey)).toString();
    const bool rememberedAvailable = !rememberedFolder.isEmpty() && QDir(rememberedFolder).exists();

    if (rememberedAvailable)
        return true;

    // A fresh install adopts Documents\SonKuPik K500\Presets as its canonical
    // local library. For a temporarily unavailable custom/removable folder we
    // use the default for this session, then restore the remembered preference
    // so the custom library can automatically return when the drive is back.
    const QUrl defaultUrl = QUrl::fromLocalFile(m_presetFolder);
    const bool invoked = QMetaObject::invokeMethod(
        presetFileBridge,
        "setPresetFolder",
        Qt::DirectConnection,
        Q_ARG(QUrl, defaultUrl));

    if (hasRememberedFolder && !rememberedFolder.isEmpty())
        settings.setValue(QString::fromLatin1(PresetFolderKey), rememberedFolder);

    return invoked;
}
