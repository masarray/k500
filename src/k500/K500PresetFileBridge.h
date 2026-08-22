#pragma once

#include <QByteArray>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QUrl>
#include <QtQml/qqmlregistration.h>

class StudioEngine;

class K500PresetFileBridge final : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QObject *engine READ engine WRITE setEngine NOTIFY engineChanged)
    Q_PROPERTY(bool loaded READ loaded NOTIFY sourceChanged)
    Q_PROPERTY(QString sourcePath READ sourcePath NOTIFY sourceChanged)
    Q_PROPERTY(QString sourceName READ sourceName NOTIFY sourceChanged)
    Q_PROPERTY(QString presetName READ presetName NOTIFY sourceChanged)
    Q_PROPERTY(bool checksumOk READ checksumOk NOTIFY sourceChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY errorChanged)

public:
    explicit K500PresetFileBridge(QObject *parent = nullptr);

    QObject *engine() const;
    void setEngine(QObject *engine);

    bool loaded() const { return !m_sourceBytes.isEmpty(); }
    QString sourcePath() const { return m_sourcePath; }
    QString sourceName() const { return m_sourceName; }
    QString presetName() const { return m_presetName; }
    bool checksumOk() const { return m_checksumOk; }
    QString lastError() const { return m_lastError; }

    Q_INVOKABLE bool loadFile(const QUrl &url);
    Q_INVOKABLE bool saveFile(const QUrl &url);
    Q_INVOKABLE void clear();
    Q_INVOKABLE QByteArray deviceSlotImage() const;

signals:
    void engineChanged();
    void sourceChanged();
    void errorChanged();
    void loadedFile(const QString &path, const QString &presetName);
    void savedFile(const QString &path);

private:
    void setError(const QString &message);
    QString localPath(const QUrl &url, bool appendExtension) const;

    QPointer<StudioEngine> m_engine;
    QByteArray m_sourceBytes;
    QString m_sourcePath;
    QString m_sourceName;
    QString m_presetName;
    bool m_checksumOk = false;
    QString m_lastError;
};
