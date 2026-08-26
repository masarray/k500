#pragma once

#include <QByteArray>
#include <QMetaObject>
#include <QObject>
#include <QString>
#include <QUrl>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

class StudioEngine;

// P3_2_FILE_BRIDGE_V1
// Native backend boundary for validated .k500 import/export. P3.4 extends this
// object with controlled edit persistence, but every mutation still goes through
// K500PresetEditMapper -> K500PresetCodec explicit byte whitelists.
//
// P6_PC_PRESET_LIBRARY_V1 adds a read-only PC-side preset catalog. It never
// mutates device slots implicitly: built-in and folder presets first hydrate the
// validated PC working document, and the existing explicit Upload/Save actions
// remain the only paths that write to K500 hardware.
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

    Q_PROPERTY(QString presetFolder READ presetFolder NOTIFY libraryChanged)
    Q_PROPERTY(QVariantList folderPresets READ folderPresets NOTIFY libraryChanged)
    Q_PROPERTY(QVariantList builtInPresets READ builtInPresets CONSTANT)

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

    QString presetFolder() const { return m_presetFolder; }
    QVariantList folderPresets() const { return m_folderPresets; }
    QVariantList builtInPresets() const { return m_builtInPresets; }

    Q_INVOKABLE bool loadFile(const QUrl &url);
    Q_INVOKABLE bool saveFile(const QUrl &url);
    Q_INVOKABLE void clear();
    Q_INVOKABLE QByteArray deviceSlotImage() const;

    // P6_PC_PRESET_LIBRARY_V1 — folder discovery is intentionally local-only
    // and read-only. Invalid files stay visible with valid=false so users can
    // diagnose a bad preset without risking hydration or device writes.
    Q_INVOKABLE bool setPresetFolder(const QUrl &url);
    Q_INVOKABLE void refreshPresetFolder();
    Q_INVOKABLE bool loadFolderPreset(int index);
    Q_INVOKABLE bool loadBuiltInPreset(int index);

    // P4_2_PRESET_BATCH_LIBRARY_V1 — validate/convert a deterministic batch of
    // local .k500 files for the already-proven P2 Mass Upload engine. Files are
    // sorted by filename and mapped sequentially from startSlotOneBased.
    Q_INVOKABLE QVariantList buildMassUploadEntries(const QVariantList &urls,
                                                     int startSlotOneBased);

signals:
    void engineChanged();
    void sourceChanged();
    void errorChanged();
    void libraryChanged();
    void loadedFile(const QString &path, const QString &presetName);
    void savedFile(const QString &path);
    void persistedEdit(const QString &path, int changedByteCount);
    void persistenceRejected(const QString &path, const QString &reason);

private:
    void setError(const QString &message);
    QString localPath(const QUrl &url, bool appendExtension) const;
    void onEngineEdit(const QString &path, const QVariant &value);
    void refreshDocumentMetadata();

    bool loadValidatedBytes(const QByteArray &bytes,
                            const QString &sourcePath,
                            const QString &sourceName);
    QVariantMap describePreset(const QByteArray &bytes,
                               const QString &displayName,
                               const QString &fileName,
                               const QString &path,
                               const QString &source,
                               int index) const;
    void rebuildBuiltInPresets();
    void rebuildFolderPresets();

    StudioEngine *m_engine = nullptr;
    QMetaObject::Connection m_engineEditConnection;
    QByteArray m_sourceBytes; // current working document, always checksum-valid
    QByteArray m_savedBytes;  // last loaded/saved checkpoint for dirty tracking
    QString m_sourcePath;
    QString m_sourceName;
    QString m_presetName;
    bool m_checksumOk = false;
    QString m_lastError;

    QString m_presetFolder;
    QVariantList m_folderPresets;
    QVariantList m_builtInPresets;
};
