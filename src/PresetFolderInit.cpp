#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>

namespace {
void initializeDefaultPresetLibrary()
{
    // USER_PRESET_LIBRARY_V1
    // Explicit org/app keys keep this startup migration deterministic even
    // though Q_COREAPP_STARTUP_FUNCTION runs before main() assigns identity.
    QSettings settings(QStringLiteral("MasArray"), QStringLiteral("SonKuPik K500"));
    const QString key = QStringLiteral("pcPresetLibrary/folder");
    const QString remembered = QDir::cleanPath(settings.value(key).toString());
    if (!remembered.isEmpty() && QDir(remembered).exists())
        return; // Respect an existing custom user library.

    QString documents = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (documents.isEmpty())
        documents = QDir::homePath() + QStringLiteral("/Documents");

    const QString presetFolder = QDir(documents).filePath(
        QStringLiteral("SonKuPik K500/Presets"));
    if (QDir().mkpath(presetFolder))
        settings.setValue(key, QDir::cleanPath(presetFolder));
}
}

Q_COREAPP_STARTUP_FUNCTION(initializeDefaultPresetLibrary)
