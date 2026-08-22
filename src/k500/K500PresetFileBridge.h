#pragma once

#include <QByteArray>
#include <QMetaObject>
#include <QObject>
#include <QString>
#include <QUrl>
#include <QVariant>
#include <QVariantList>

class StudioEngine;

// P3_2_FILE_BRIDGE_V1
// Native backend boundary for validated .k500 import/export. P3.4 extends this
// object with controlled edit persistence, but every mutation still goes through
// K500PresetEditMapper -> K500PresetCodec explicit byte whitelists.
class K500PresetFileBridge final : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QObject *engine READ engine WRITE setEngine NOTIFY engineChanged)
    Q_PROPERTY(bool loaded READ loaded NOTIFY sourceChanged)
    Q_PROPERTY(bool dirty READ dirty NOTIFY sourceChanged)
    Q_PROPERTY(bool editPersistenceEnabled READ editPersistenceEnabled NOTIFY sourceChanged)
    Q_PROPERTY(QString sourcePath READ sourcePath NOTIFY sourceChanged)
    Q_PROPERTY(QString sourceName READ sourceName NOTIFY sourceChanged)
    Q_PROPERTY(QString presetName READ presetName NOTIFY sourceChanged)
    Q_PROPERTY(bool checksumOk READ checksumOk NOTIFY sourceChanged)
    Q_PROPERTY(int changedByteCount READ changedByteCount NOTIFY sourceChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY errorChanged)

public:
    explicit K500PresetFileBridge(QObject *parent = nullptr);

    QObject *engine() const;
    void setEngine(QObject *engine);

    bool loaded() const { return m_sourceBytes.size() == 0x0478; }
    bool dirty() const { return loaded() && m_sourceBytes != m_savedBytes; }
    bool editPersistenceEnabled() const { return loaded() && m_engine != nullptr; }
    QString sourcePath() const { return m_sourcePath; }
    QString sourceName() const { return m_sourceName; }
    QString presetName() const { return m_presetName; }
    bool checksumOk() const { return m_checksumOk; }
    int changedByteCount() const;
    QString lastError() const { return m_lastError; }

    Q_INVOKABLE bool loadFile(const QUrl &url);
    Q_INVOKABLE bool saveFile(const QUrl &url);
    Q_INVOKABLE void clear();
    Q_INVOKABLE QByteArray deviceSlotImage() const;

    // P4_2_PRESET_BATCH_LIBRARY_V1 — validate/convert a deterministic batch of
    // local .k500 files for the already-proven P2 Mass Upload engine. Files are
    // sorted by filename and mapped sequentially from startSlotOneBased.
    Q_INVOKABLE QVariantList buildMassUploadEntries(const QVariantList &urls,
                                                     int startSlotOneBased);

signals:
    void engineChanged();
    void sourceChanged();
    void errorChanged();
    void loadedFile(const QString &path, const QString &presetName);
    void savedFile(const QString &path);
    void persistedEdit(const QString &path, int changedByteCount);
    void persistenceRejected(const QString &path, const QString &reason);

private:
    void setError(const QString &message);
    QString localPath(const QUrl &url, bool appendExtension) const;
    void onEngineEdit(const QString &path, const QVariant &value);
    void refreshDocumentMetadata();

    StudioEngine *m_engine = nullptr;
    QMetaObject::Connection m_engineEditConnection;
    QByteArray m_sourceBytes; // current working document, always checksum-valid
    QByteArray m_savedBytes;  // last loaded/saved checkpoint for dirty tracking
    QString m_sourcePath;
    QString m_sourceName;
    QString m_presetName;
    bool m_checksumOk = false;
    QString m_lastError;
};
