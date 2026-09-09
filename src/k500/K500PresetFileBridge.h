#pragma once

#include <QByteArray>
#include <QMetaObject>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

class QNetworkAccessManager;
class StudioEngine;

// P3_2_FILE_BRIDGE_V1
// Native backend boundary for validated .k500 import/export. P3.4 extends this
// object with controlled edit persistence, but every mutation still goes through
// K500PresetEditMapper -> K500PresetCodec explicit byte whitelists.
//
// P6_PC_PRESET_LIBRARY_V1
// Official SonKuPik presets and user-local presets remain PC-side data. Selection
// is staging-only; hardware changes only through the explicit upload manager.
class K500PresetFileBridge final : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QObject *engine READ engine WRITE setEngine NOTIFY engineChanged)
    Q_PROPERTY(bool editTracking READ editTracking WRITE setEditTracking NOTIFY editTrackingChanged)
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
    // Historic property retained for P6/UI compatibility. Its content is now the
    // current official SonKuPik library: bundled fallback plus validated cache.
    Q_PROPERTY(QVariantList builtInPresets READ builtInPresets NOTIFY libraryChanged)
    Q_PROPERTY(QVariantList combinedPresets READ combinedPresets NOTIFY libraryChanged)

    Q_PROPERTY(bool officialSyncBusy READ officialSyncBusy NOTIFY officialSyncChanged)
    Q_PROPERTY(QString officialSyncStatus READ officialSyncStatus NOTIFY officialSyncChanged)
    Q_PROPERTY(QString officialSyncError READ officialSyncError NOTIFY officialSyncChanged)
    Q_PROPERTY(int officialUpdateCount READ officialUpdateCount NOTIFY officialSyncChanged)

public:
    explicit K500PresetFileBridge(QObject *parent = nullptr);

    QObject *engine() const;
    void setEngine(QObject *engine);
    bool editTracking() const { return m_editTracking; }
    void setEditTracking(bool enabled);

    bool loaded() const { return m_sourceBytes.size() == 0x0478; }
    bool dirty() const { return loaded() && m_sourceBytes != m_savedBytes; }
    bool editPersistenceEnabled() const { return loaded() && m_engine != nullptr && m_editTracking; }
    QString sourcePath() const { return m_sourcePath; }
    QString sourceName() const { return m_sourceName; }
    QString presetName() const { return m_presetName; }
    bool checksumOk() const { return m_checksumOk; }
    int changedByteCount() const;
    QString lastError() const { return m_lastError; }

    QString presetFolder() const { return m_presetFolder; }
    QVariantList folderPresets() const { return m_folderPresets; }
    QVariantList builtInPresets() const { return m_builtInPresets; }
    QVariantList combinedPresets() const { return m_combinedPresets; }

    bool officialSyncBusy() const { return m_officialSyncBusy; }
    QString officialSyncStatus() const { return m_officialSyncStatus; }
    QString officialSyncError() const { return m_officialSyncError; }
    int officialUpdateCount() const { return m_officialUpdateCount; }

    Q_INVOKABLE bool loadFile(const QUrl &url);
    Q_INVOKABLE bool saveFile(const QUrl &url);
    Q_INVOKABLE void clear();
    Q_INVOKABLE QByteArray deviceSlotImage() const;

    // OFFLINE_PREVIEW_V1 — explicit user action may hydrate editor state while
    // disconnected. Merely staging a file never hydrates StudioEngine.
    Q_INVOKABLE bool previewLoadedPreset();

    Q_INVOKABLE bool setPresetFolder(const QUrl &url);
    Q_INVOKABLE void refreshPresetFolder();
    Q_INVOKABLE bool loadFolderPreset(int index);
    Q_INVOKABLE bool loadBuiltInPreset(int index);

    // OFFICIAL_PRESET_SYNC_V1 — fetch only new/changed official files, validate
    // exact K500 size/checksum, then atomically replace cache. Network failure
    // never removes the bundled or last-known-good library.
    Q_INVOKABLE void syncOfficialPresets();

    // P4_2_PRESET_BATCH_LIBRARY_V1 — legacy deterministic builder retained for
    // regression parity with the previous file-dialog path.
    Q_INVOKABLE QVariantList buildMassUploadEntries(const QVariantList &urls,
                                                     int startSlotOneBased);

    // SYSTEM_TRANSFER_LIST_V1 — visible right-list row 01..10 maps exactly to
    // device slot 01..10. Transport later sends those slots in native 10->1 order.
    Q_INVOKABLE QVariantList buildTransferUploadEntries(const QVariantList &paths);

signals:
    void engineChanged();
    void editTrackingChanged();
    void sourceChanged();
    void errorChanged();
    void libraryChanged();
    void officialSyncChanged();
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
    void rebuildCombinedPresets();

    QString officialCacheDirectory() const;
    void setOfficialSyncState(bool busy, const QString &status, const QString &error = {});
    void downloadNextOfficialPreset();
    void finishOfficialSync();

    StudioEngine *m_engine = nullptr;
    QMetaObject::Connection m_engineEditConnection;
    bool m_editTracking = false;
    QByteArray m_sourceBytes;
    QByteArray m_savedBytes;
    QString m_sourcePath;
    QString m_sourceName;
    QString m_presetName;
    bool m_checksumOk = false;
    QString m_lastError;

    QString m_presetFolder;
    QVariantList m_folderPresets;
    QVariantList m_builtInPresets;
    QVariantList m_combinedPresets;

    QNetworkAccessManager *m_networkManager = nullptr;
    bool m_officialSyncBusy = false;
    QString m_officialSyncStatus = QStringLiteral("Bundled SonKuPik presets ready");
    QString m_officialSyncError;
    int m_officialUpdateCount = 0;
    int m_officialSyncTotal = 0;
    QVariantList m_officialDownloadQueue;
    QStringList m_officialRemoteNames;
};
