#pragma once

#include <QAbstractListModel>
#include <QByteArray>
#include <QList>
#include <QObject>
#include <QStringList>
#include <QVariantMap>

class EqBandModel final : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count CONSTANT)
    Q_PROPERTY(double hpfHz READ hpfHz NOTIFY crossoverChanged)
    Q_PROPERTY(double lpfHz READ lpfHz NOTIFY crossoverChanged)
    Q_PROPERTY(QString hpType READ hpType NOTIFY crossoverChanged)
    Q_PROPERTY(QString lpType READ lpType NOTIFY crossoverChanged)

public:
    enum Role {
        FrequencyRole = Qt::UserRole + 1,
        GainRole,
        QRole,
        TypeNameRole
    };

    explicit EqBandModel(QObject *parent = nullptr);
    int count() const { return m_bands.size(); }
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    double hpfHz() const { return m_hpfHz; }
    double lpfHz() const { return m_lpfHz; }
    QString hpType() const { return m_hpType; }
    QString lpType() const { return m_lpType; }

    Q_INVOKABLE QVariantMap get(int index) const;
    Q_INVOKABLE void setBand(int index, double frequency, double gain, double q);
    Q_INVOKABLE void setBandType(int index, const QString &typeName);
    Q_INVOKABLE void resetBand(int index);
    Q_INVOKABLE void resetAll();
    Q_INVOKABLE void setHpfHz(double value);
    Q_INVOKABLE void setLpfHz(double value);
    Q_INVOKABLE void setHpType(const QString &value);
    Q_INVOKABLE void setLpType(const QString &value);

    void configure(int bandCount, const QList<double> &defaultFrequencies,
                   double hpfHz, double lpfHz,
                   const QString &hpType, const QString &lpType);
    void syncBand(int index, double frequency, double gain, double q, const QString &typeName);
    void syncCrossover(double hpfHz, double lpfHz,
                       const QString &hpType, const QString &lpType);

signals:
    void bandChanged(int index, double frequency, double gain, double q, const QString &typeName);
    void crossoverChanged();
    void crossoverEditRequested(const QString &field, const QVariant &value);

private:
    struct Band {
        double frequency;
        double gain;
        double q;
        QString typeName;
    };
    QList<Band> m_bands;
    QList<double> m_defaultFrequencies{80, 160, 315, 630, 1300, 2500, 8000};
    double m_hpfHz = 20.0;
    double m_lpfHz = 20000.0;
    QString m_hpType = QStringLiteral("HP Butter 12");
    QString m_lpType = QStringLiteral("LP Butter 12");
};

class StudioEngine final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(EqBandModel *musicEqBands READ musicEqBands CONSTANT)
    Q_PROPERTY(EqBandModel *micAEqBands READ micAEqBands CONSTANT)
    Q_PROPERTY(EqBandModel *micBEqBands READ micBEqBands CONSTANT)
    Q_PROPERTY(EqBandModel *reverbEqBands READ reverbEqBands CONSTANT)
    Q_PROPERTY(EqBandModel *echoEqBands READ echoEqBands CONSTANT)
    Q_PROPERTY(EqBandModel *mainEqBands READ mainEqBands CONSTANT)
    Q_PROPERTY(EqBandModel *surroundEqBands READ surroundEqBands CONSTANT)
    Q_PROPERTY(EqBandModel *centerEqBands READ centerEqBands CONSTANT)
    Q_PROPERTY(EqBandModel *subEqBands READ subEqBands CONSTANT)
    Q_PROPERTY(QVariantMap deviceState READ deviceState NOTIFY deviceStateChanged)
    Q_PROPERTY(bool deviceStateReady READ deviceStateReady NOTIFY deviceStateChanged)
    Q_PROPERTY(int musicKey READ musicKey WRITE setMusicKey NOTIFY musicKeyChanged)
    Q_PROPERTY(double noiseGate READ noiseGate WRITE setNoiseGate NOTIFY noiseGateChanged)
    Q_PROPERTY(double bass READ bass WRITE setBass NOTIFY bassChanged)
    Q_PROPERTY(double mid READ mid WRITE setMid NOTIFY midChanged)
    Q_PROPERTY(double midFreq READ midFreq WRITE setMidFreq NOTIFY midFreqChanged)
    Q_PROPERTY(double treble READ treble WRITE setTreble NOTIFY trebleChanged)
    Q_PROPERTY(double hpfHz READ hpfHz WRITE setHpfHz NOTIFY hpfHzChanged)
    Q_PROPERTY(double lpfHz READ lpfHz WRITE setLpfHz NOTIFY lpfHzChanged)
    Q_PROPERTY(QString hpType READ hpType WRITE setHpType NOTIFY hpTypeChanged)
    Q_PROPERTY(QString lpType READ lpType WRITE setLpType NOTIFY lpTypeChanged)
    Q_PROPERTY(double input1Gain READ input1Gain WRITE setInput1Gain NOTIFY input1GainChanged)
    Q_PROPERTY(double input2Gain READ input2Gain WRITE setInput2Gain NOTIFY input2GainChanged)
    Q_PROPERTY(double bluetoothGain READ bluetoothGain WRITE setBluetoothGain NOTIFY bluetoothGainChanged)
    Q_PROPERTY(double uDiskGain READ uDiskGain WRITE setUDiskGain NOTIFY uDiskGainChanged)
    Q_PROPERTY(double digitalGain READ digitalGain WRITE setDigitalGain NOTIFY digitalGainChanged)
    Q_PROPERTY(double masterMusic READ masterMusic WRITE setMasterMusic NOTIFY masterMusicChanged)
    Q_PROPERTY(double masterMic READ masterMic WRITE setMasterMic NOTIFY masterMicChanged)
    Q_PROPERTY(double masterFx READ masterFx WRITE setMasterFx NOTIFY masterFxChanged)
    Q_PROPERTY(QString lastChangedPath READ lastChangedPath NOTIFY stateEdited)

public:
    explicit StudioEngine(QObject *parent = nullptr);

    EqBandModel *musicEqBands() { return &m_musicEqBands; }
    EqBandModel *micAEqBands() { return &m_micAEqBands; }
    EqBandModel *micBEqBands() { return &m_micBEqBands; }
    EqBandModel *reverbEqBands() { return &m_reverbEqBands; }
    EqBandModel *echoEqBands() { return &m_echoEqBands; }
    EqBandModel *mainEqBands() { return &m_mainEqBands; }
    EqBandModel *surroundEqBands() { return &m_surroundEqBands; }
    EqBandModel *centerEqBands() { return &m_centerEqBands; }
    EqBandModel *subEqBands() { return &m_subEqBands; }
    QVariantMap deviceState() const { return m_deviceState; }
    bool deviceStateReady() const { return m_deviceStateReady; }

    int musicKey() const { return m_musicKey; }
    double noiseGate() const { return m_noiseGate; }
    double bass() const { return m_bass; }
    double mid() const { return m_mid; }
    double midFreq() const { return m_midFreq; }
    double treble() const { return m_treble; }
    double hpfHz() const { return m_hpfHz; }
    double lpfHz() const { return m_lpfHz; }
    QString hpType() const { return m_hpType; }
    QString lpType() const { return m_lpType; }
    double input1Gain() const { return m_input1Gain; }
    double input2Gain() const { return m_input2Gain; }
    double bluetoothGain() const { return m_bluetoothGain; }
    double uDiskGain() const { return m_uDiskGain; }
    double digitalGain() const { return m_digitalGain; }
    double masterMusic() const { return m_masterMusic; }
    double masterMic() const { return m_masterMic; }
    double masterFx() const { return m_masterFx; }
    QString lastChangedPath() const { return m_lastChangedPath; }

public slots:
    void hydrateFromDeviceMemory(const QByteArray &memory);
    void clearDeviceState();

    // P1_CANONICAL_DEVICE_EDIT_V1 — rack controls still enter through
    // StudioEngine; QML never bypasses the architecture boundary to Controller/I/O.
    void editDevicePath(const QString &path, const QVariant &value)
    {
        if (path.isEmpty())
            return;
        m_lastChangedPath = path;
        emit stateEdited(path, value);
    }

    void setMusicKey(int value);
    void setNoiseGate(double value);
    void setBass(double value);
    void setMid(double value);
    void setMidFreq(double value);
    void setTreble(double value);
    void setHpfHz(double value);
    void setLpfHz(double value);
    void setHpType(const QString &value);
    void setLpType(const QString &value);
    void setInput1Gain(double value);
    void setInput2Gain(double value);
    void setBluetoothGain(double value);
    void setUDiskGain(double value);
    void setDigitalGain(double value);
    void setMasterMusic(double value);
    void setMasterMic(double value);
    void setMasterFx(double value);

signals:
    void deviceStateChanged();
    void musicKeyChanged();
    void noiseGateChanged();
    void bassChanged();
    void midChanged();
    void midFreqChanged();
    void trebleChanged();
    void hpfHzChanged();
    void lpfHzChanged();
    void hpTypeChanged();
    void lpTypeChanged();
    void input1GainChanged();
    void input2GainChanged();
    void bluetoothGainChanged();
    void uDiskGainChanged();
    void digitalGainChanged();
    void masterMusicChanged();
    void masterMicChanged();
    void masterFxChanged();
    void stateEdited(const QString &path, const QVariant &value);

private:
    template<typename T>
    bool assign(T &target, const T &value, const char *path)
    {
        if (target == value)
            return false;
        target = value;
        m_lastChangedPath = QString::fromLatin1(path);
        emit stateEdited(m_lastChangedPath, QVariant::fromValue(value));
        return true;
    }

    void connectEqModel(EqBandModel *model, const QString &key);
    void syncMusicCrossoverModel();

    EqBandModel m_musicEqBands;
    EqBandModel m_micAEqBands;
    EqBandModel m_micBEqBands;
    EqBandModel m_reverbEqBands;
    EqBandModel m_echoEqBands;
    EqBandModel m_mainEqBands;
    EqBandModel m_surroundEqBands;
    EqBandModel m_centerEqBands;
    EqBandModel m_subEqBands;

    QVariantMap m_deviceState;
    bool m_deviceStateReady = false;

    int m_musicKey = 0;
    double m_noiseGate = -70.0;
    double m_bass = 0.0;
    double m_mid = 0.0;
    double m_midFreq = 1000.0;
    double m_treble = 0.0;
    double m_hpfHz = 20.0;
    double m_lpfHz = 20000.0;
    QString m_hpType = QStringLiteral("HP Butter 12");
    QString m_lpType = QStringLiteral("LP Butter 12");
    double m_input1Gain = -3.0;
    double m_input2Gain = -3.0;
    double m_bluetoothGain = -3.0;
    double m_uDiskGain = -4.0;
    double m_digitalGain = -4.0;
    double m_masterMusic = 35.0;
    double m_masterMic = 35.0;
    double m_masterFx = 35.0;
    QString m_lastChangedPath;
};
