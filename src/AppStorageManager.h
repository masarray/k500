#pragma once

#include <QObject>
#include <QString>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

class AppStorageManager final : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString documentsRoot READ documentsRoot CONSTANT)
    Q_PROPERTY(QString presetFolder READ presetFolder CONSTANT)
    Q_PROPERTY(QString backupFolder READ backupFolder CONSTANT)
    Q_PROPERTY(QString exportFolder READ exportFolder CONSTANT)
    Q_PROPERTY(QUrl presetFolderUrl READ presetFolderUrl CONSTANT)

public:
    explicit AppStorageManager(QObject *parent = nullptr);

    QString documentsRoot() const { return m_documentsRoot; }
    QString presetFolder() const { return m_presetFolder; }
    QString backupFolder() const { return m_backupFolder; }
    QString exportFolder() const { return m_exportFolder; }
    QUrl presetFolderUrl() const { return QUrl::fromLocalFile(m_presetFolder); }

    // Creates the user-visible storage tree and gives the existing validated
    // preset bridge a deterministic default library only when the user has not
    // already chosen a custom folder. If a remembered removable/custom folder
    // is temporarily unavailable, the session safely falls back to Documents
    // without destroying the remembered preference.
    Q_INVOKABLE bool ensureUserStorage(QObject *presetFileBridge);

private:
    QString m_documentsRoot;
    QString m_presetFolder;
    QString m_backupFolder;
    QString m_exportFolder;
};
