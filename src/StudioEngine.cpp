#include "StudioEngine.h"

#include <QtMath>

namespace {
constexpr int ActiveMemorySize = 0x03AB;
constexpr int LiveScalarSplit = 0x008F;
constexpr int FileScalarLowBase = 0x0008;
constexpr int FileScalarHighBase = 0x0098;

struct LiveEqDescriptor {
    const char *key;
    int liveOffset;
    int bandCount;
    int hpfFileOffset;
    int lpfFileOffset;
};

constexpr LiveEqDescriptor LiveEqSections[] = {
    {"micA",      0x00E7, 10, 0x0098, 0x009A},
    {"micB",      0x0119, 10, 0x0098, 0x009A},
    {"music",     0x014B,  7, 0x009C, 0x009E},
    {"main",      0x016E,  7, 0x00A0, 0x00A4},
    {"mainAlt",   0x0191,  7, 0x00A2, 0x00A6},
    {"surround",  0x01B4,  5, 0x00A8, 0x00AC},
    {"surroundAlt",0x01CD, 5, 0x00AA, 0x00AE},
    {"center",    0x01E6,  5, 0x00B0, 0x00B4},
    {"centerAlt", 0x01FF,  5, 0x00B2, 0x00B6},
    {"sub",       0x0218,  5, 0x00B8, 0x00BC},
    {"subAlt",    0x0231,  5, 0x00BA, 0x00BE},
    {"reverb",    0x024A,  5, 0x00C0, 0x00C2},
    {"echo",      0x0263,  5, 0x00C4, 0x00C6},
};

double clampValue(double value, double minimum, double maximum)
{
    return qBound(minimum, value, maximum);
}

QString normalizeBandType(const QString &value)
{
    const QString upper = value.trimmed().toUpper();
    if (upper == QStringLiteral("LS") || upper == QStringLiteral("LOW SHELF") || upper == QStringLiteral("LOWSHELF"))
        return QStringLiteral("LOW SHELF");
    if (upper == QStringLiteral("HS") || upper == QStringLiteral("HIGH SHELF") || upper == QStringLiteral("HIGHSHELF"))
        return QStringLiteral("HIGH SHELF");
    return QStringLiteral("BELL");
}

int liveOffsetForFileScalar(int fileOffset)
{
    // Donor/native capture parity:
    // live[0x0000..0x008e] == file[0x0008..0x0096]
    // live[0x008f..0x00e6] == file[0x0098..0x00ef]
    // File byte 0x0097 has no live counterpart.
    if (fileOffset >= FileScalarLowBase && fileOffset <= 0x0096)
        return fileOffset - 0x08;
    if (fileOffset >= FileScalarHighBase && fileOffset <= 0x00EF)
        return fileOffset - 0x09;
    return -1;
}

quint8 byteAt(const QByteArray &memory, int offset, quint8 fallback = 0)
{
    if (offset < 0 || offset >= memory.size())
        return fallback;
    return static_cast<quint8>(static_cast<unsigned char>(memory.at(offset)));
}

quint16 u16At(const QByteArray &memory, int offset, quint16 fallback = 0)
{
    if (offset < 0 || offset + 1 >= memory.size())
        return fallback;
    return static_cast<quint16>(byteAt(memory, offset)
        | (static_cast<quint16>(byteAt(memory, offset + 1)) << 8));
}

quint8 fileU8(const QByteArray &memory, int fileOffset, quint8 fallback = 0)
{
    return byteAt(memory, liveOffsetForFileScalar(fileOffset), fallback);
}

quint16 fileU16(const QByteArray &memory, int fileOffset, quint16 fallback = 0)
{
    const int liveOffset = liveOffsetForFileScalar(fileOffset);
    if (liveOffset < 0)
        return fallback;
    return u16At(memory, liveOffset, fallback);
}

double outputDb(quint8 raw)
{
    return (static_cast<int>(raw) - 75) / 2.0;
}

QString fixedAscii(const QByteArray &memory, int offset, int length)
{
    if (offset < 0 || offset >= memory.size() || length <= 0)
        return {};
    QString text;
    const int end = qMin(memory.size(), offset + length);
    for (int i = offset; i < end; ++i) {
        const quint8 value = byteAt(memory, i);
        if (value == 0)
            break;
        if (value >= 0x20 && value <= 0x7E)
            text.append(QChar(value));
    }
    return text.trimmed();
}

QString bandTypeFromLive(quint8 typeSign)
{
    switch (typeSign & 0x70) {
    case 0x10: return QStringLiteral("LOW SHELF");
    case 0x20: return QStringLiteral("HIGH SHELF");
    default: return QStringLiteral("BELL");
    }
}

QVariantMap compState(const QByteArray &memory, int thresholdOffset)
{
    return {
        {QStringLiteral("compThresholdDb"), static_cast<int>(fileU8(memory, thresholdOffset)) - 50},
        {QStringLiteral("compRatio"), static_cast<int>(fileU8(memory, thresholdOffset + 1))},
        {QStringLiteral("attackMs"), static_cast<int>(fileU8(memory, thresholdOffset + 2))},
        {QStringLiteral("releaseSec"), fileU8(memory, thresholdOffset + 3) / 10.0},
    };
}

void mergeMap(QVariantMap &target, const QVariantMap &source)
{
    for (auto it = source.constBegin(); it != source.constEnd(); ++it)
        target.insert(it.key(), it.value());
}
}

EqBandModel::EqBandModel(QObject *parent)
    : QAbstractListModel(parent)
{
    resetAll();
}

int EqBandModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_bands.size();
}

QVariant EqBandModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_bands.size())
        return {};
    const auto &band = m_bands.at(index.row());
    switch (role) {
    case FrequencyRole: return band.frequency;
    case GainRole: return band.gain;
    case QRole: return band.q;
    case TypeNameRole: return band.typeName;
    default: return {};
    }
}

QHash<int, QByteArray> EqBandModel::roleNames() const
{
    return {{FrequencyRole, "freq"}, {GainRole, "gain"}, {QRole, "q"}, {TypeNameRole, "typeName"}};
}

QVariantMap EqBandModel::get(int index) const
{
    if (index < 0 || index >= m_bands.size())
        return {};
    const auto &band = m_bands.at(index);
    return {{QStringLiteral("freq"), band.frequency},
            {QStringLiteral("frequency"), band.frequency},
            {QStringLiteral("gain"), band.gain},
            {QStringLiteral("q"), band.q},
            {QStringLiteral("typeName"), band.typeName}};
}

void EqBandModel::configure(int bandCount, const QList<double> &defaultFrequencies,
                            double hpfHz, double lpfHz,
                            const QString &hpType, const QString &lpType)
{
    m_defaultFrequencies = defaultFrequencies;
    if (m_defaultFrequencies.size() != bandCount) {
        m_defaultFrequencies.clear();
        for (int i = 0; i < bandCount; ++i)
            m_defaultFrequencies.append(1000.0);
    }
    m_hpfHz = hpfHz;
    m_lpfHz = lpfHz;
    m_hpType = hpType;
    m_lpType = lpType;
    resetAll();
}

void EqBandModel::setBand(int index, double frequency, double gain, double q)
{
    if (index < 0 || index >= m_bands.size())
        return;
    auto &band = m_bands[index];
    const double nextFrequency = clampValue(frequency, 20.0, 20000.0);
    const double nextGain = clampValue(gain, -24.0, 24.0);
    const double nextQ = clampValue(q, 0.1, 30.0);
    if (qFuzzyCompare(band.frequency, nextFrequency)
        && qFuzzyCompare(band.gain + 25.0, nextGain + 25.0)
        && qFuzzyCompare(band.q, nextQ))
        return;
    band.frequency = nextFrequency;
    band.gain = nextGain;
    band.q = nextQ;
    emit dataChanged(this->index(index), this->index(index), {FrequencyRole, GainRole, QRole});
    emit bandChanged(index, band.frequency, band.gain, band.q, band.typeName);
}

void EqBandModel::setBandType(int index, const QString &typeName)
{
    if (index < 0 || index >= m_bands.size())
        return;
    auto &band = m_bands[index];
    const QString normalized = normalizeBandType(typeName);
    if (band.typeName == normalized)
        return;
    band.typeName = normalized;
    emit dataChanged(this->index(index), this->index(index), {TypeNameRole});
    emit bandChanged(index, band.frequency, band.gain, band.q, band.typeName);
}

void EqBandModel::syncBand(int index, double frequency, double gain, double q, const QString &typeName)
{
    if (index < 0 || index >= m_bands.size())
        return;
    auto &band = m_bands[index];
    const double nextFrequency = clampValue(frequency, 20.0, 20000.0);
    const double nextGain = clampValue(gain, -24.0, 24.0);
    const double nextQ = clampValue(q, 0.1, 30.0);
    const QString nextType = normalizeBandType(typeName);
    if (qFuzzyCompare(band.frequency, nextFrequency)
        && qFuzzyCompare(band.gain + 25.0, nextGain + 25.0)
        && qFuzzyCompare(band.q, nextQ)
        && band.typeName == nextType)
        return;
    band.frequency = nextFrequency;
    band.gain = nextGain;
    band.q = nextQ;
    band.typeName = nextType;
    emit dataChanged(this->index(index), this->index(index),
                     {FrequencyRole, GainRole, QRole, TypeNameRole});
}

void EqBandModel::resetBand(int index)
{
    if (index < 0 || index >= m_bands.size())
        return;
    const double frequency = index < m_defaultFrequencies.size() ? m_defaultFrequencies.at(index) : 1000.0;
    setBand(index, frequency, 0.0, 1.0);
    setBandType(index, QStringLiteral("BELL"));
}

void EqBandModel::resetAll()
{
    beginResetModel();
    m_bands.clear();
    for (const double frequency : std::as_const(m_defaultFrequencies))
        m_bands.append({frequency, 0.0, 1.0, QStringLiteral("BELL")});
    endResetModel();
    emit crossoverChanged();
}

void EqBandModel::setHpfHz(double value)
{
    const double next = clampValue(value, 20.0, 20000.0);
    if (qFuzzyCompare(m_hpfHz, next))
        return;
    m_hpfHz = next;
    emit crossoverChanged();
    emit crossoverEditRequested(QStringLiteral("hpfHz"), next);
}

void EqBandModel::setLpfHz(double value)
{
    const double next = clampValue(value, 20.0, 20000.0);
    if (qFuzzyCompare(m_lpfHz, next))
        return;
    m_lpfHz = next;
    emit crossoverChanged();
    emit crossoverEditRequested(QStringLiteral("lpfHz"), next);
}

void EqBandModel::setHpType(const QString &value)
{
    if (m_hpType == value)
        return;
    m_hpType = value;
    emit crossoverChanged();
    emit crossoverEditRequested(QStringLiteral("hpType"), value);
}

void EqBandModel::setLpType(const QString &value)
{
    if (m_lpType == value)
        return;
    m_lpType = value;
    emit crossoverChanged();
    emit crossoverEditRequested(QStringLiteral("lpType"), value);
}

void EqBandModel::syncCrossover(double hpfHz, double lpfHz,
                                const QString &hpType, const QString &lpType)
{
    const double nextHpf = clampValue(hpfHz, 20.0, 20000.0);
    const double nextLpf = clampValue(lpfHz, 20.0, 20000.0);
    if (qFuzzyCompare(m_hpfHz, nextHpf)
        && qFuzzyCompare(m_lpfHz, nextLpf)
        && m_hpType == hpType
        && m_lpType == lpType)
        return;
    m_hpfHz = nextHpf;
    m_lpfHz = nextLpf;
    m_hpType = hpType;
    m_lpType = lpType;
    emit crossoverChanged();
}

StudioEngine::StudioEngine(QObject *parent)
    : QObject(parent)
{
    m_musicEqBands.configure(7, {80, 160, 315, 630, 1300, 2500, 8000},
                             20, 20000, QStringLiteral("HP Butter 12"), QStringLiteral("LP Butter 12"));
    m_micAEqBands.configure(10, {80, 125, 250, 500, 1000, 2000, 4000, 6300, 10000, 12500},
                            20, 20000, QStringLiteral("HP LR 24"), QStringLiteral("LP LR 24"));
    m_micBEqBands.configure(10, {80, 125, 250, 500, 1000, 2000, 4000, 6300, 10000, 12500},
                            20, 20000, QStringLiteral("HP LR 24"), QStringLiteral("LP LR 24"));
    m_reverbEqBands.configure(5, {125, 250, 1000, 2500, 8000},
                              217, 12000, QStringLiteral("HP Butter 12"), QStringLiteral("LP Butter 12"));
    m_echoEqBands.configure(5, {125, 250, 1000, 2500, 8000},
                            700, 4400, QStringLiteral("HP Butter 12"), QStringLiteral("LP Butter 12"));
    m_mainEqBands.configure(7, {80, 160, 315, 630, 1250, 2500, 8000},
                            20, 20000, QStringLiteral("HP Butter 12"), QStringLiteral("LP Butter 12"));
    m_surroundEqBands.configure(5, {125, 250, 1000, 2500, 8000},
                                20, 20000, QStringLiteral("HP Bessel 12"), QStringLiteral("LP Bessel 12"));
    m_centerEqBands.configure(5, {125, 250, 1000, 2500, 8000},
                              20, 20000, QStringLiteral("HP Butter 12"), QStringLiteral("LP Butter 12"));
    m_subEqBands.configure(5, {40, 55, 70, 85, 100},
                           40, 95, QStringLiteral("HP Butter 24"), QStringLiteral("LP Butter 24"));

    connectEqModel(&m_musicEqBands, QStringLiteral("music"));
    connectEqModel(&m_micAEqBands, QStringLiteral("micA"));
    connectEqModel(&m_micBEqBands, QStringLiteral("micB"));
    connectEqModel(&m_reverbEqBands, QStringLiteral("reverb"));
    connectEqModel(&m_echoEqBands, QStringLiteral("echo"));
    connectEqModel(&m_mainEqBands, QStringLiteral("main"));
    connectEqModel(&m_surroundEqBands, QStringLiteral("surround"));
    connectEqModel(&m_centerEqBands, QStringLiteral("center"));
    connectEqModel(&m_subEqBands, QStringLiteral("sub"));
    syncMusicCrossoverModel();
}

void StudioEngine::connectEqModel(EqBandModel *model, const QString &key)
{
    connect(model, &EqBandModel::bandChanged, this,
            [this, key](int index, double frequency, double gain, double q, const QString &typeName) {
        m_lastChangedPath = QStringLiteral("eq.%1.bands.%2").arg(key).arg(index);
        emit stateEdited(m_lastChangedPath,
                         QVariantMap{{QStringLiteral("frequency"), frequency},
                                     {QStringLiteral("gain"), gain},
                                     {QStringLiteral("q"), q},
                                     {QStringLiteral("type"), typeName}});
    });
    connect(model, &EqBandModel::crossoverEditRequested, this,
            [this, model, key](const QString &field, const QVariant &value) {
        if (key == QStringLiteral("music")) {
            if (field == QStringLiteral("hpfHz")) {
                m_hpfHz = model->hpfHz();
                emit hpfHzChanged();
            } else if (field == QStringLiteral("lpfHz")) {
                m_lpfHz = model->lpfHz();
                emit lpfHzChanged();
            } else if (field == QStringLiteral("hpType")) {
                m_hpType = model->hpType();
                emit hpTypeChanged();
            } else if (field == QStringLiteral("lpType")) {
                m_lpType = model->lpType();
                emit lpTypeChanged();
            }
        }
        m_lastChangedPath = QStringLiteral("eq.%1.crossover.%2").arg(key, field);
        emit stateEdited(m_lastChangedPath, value);
    });
}

void StudioEngine::hydrateFromDeviceMemory(const QByteArray &memory)
{
    // K500_FULL_READBACK_SYNC_V1 — exact donor/native active-memory size.
    if (memory.size() < ActiveMemorySize)
        return;

    const auto syncInt = [](int &target, int value, const auto &notify) {
        if (target == value) return;
        target = value;
        notify();
    };
    const auto syncDouble = [](double &target, double value, const auto &notify) {
        if (qFuzzyCompare(target + 1000.0, value + 1000.0)) return;
        target = value;
        notify();
    };

    syncDouble(m_masterMusic, fileU8(memory, 0x0008), [this] { emit masterMusicChanged(); });
    syncDouble(m_masterMic, fileU8(memory, 0x0009), [this] { emit masterMicChanged(); });
    syncDouble(m_masterFx, fileU8(memory, 0x000A), [this] { emit masterFxChanged(); });
    syncInt(m_musicKey, static_cast<int>(fileU8(memory, 0x0011)) - 7, [this] { emit musicKeyChanged(); });
    syncDouble(m_input1Gain, static_cast<int>(fileU8(memory, 0x001E)) - 12, [this] { emit input1GainChanged(); });
    syncDouble(m_input2Gain, static_cast<int>(fileU8(memory, 0x001F)) - 12, [this] { emit input2GainChanged(); });
    syncDouble(m_bluetoothGain, static_cast<int>(fileU8(memory, 0x0020)) - 12, [this] { emit bluetoothGainChanged(); });
    syncDouble(m_uDiskGain, static_cast<int>(fileU8(memory, 0x0021)) - 12, [this] { emit uDiskGainChanged(); });
    syncDouble(m_digitalGain, static_cast<int>(fileU8(memory, 0x0022)) - 12, [this] { emit digitalGainChanged(); });
    syncDouble(m_hpfHz, fileU16(memory, 0x009C), [this] { emit hpfHzChanged(); });
    syncDouble(m_lpfHz, fileU16(memory, 0x009E), [this] { emit lpfHzChanged(); });

    QVariantMap eqState;
    for (const LiveEqDescriptor &section : LiveEqSections) {
        const QString key = QString::fromLatin1(section.key);
        EqBandModel *model = nullptr;
        if (key == QStringLiteral("music")) model = &m_musicEqBands;
        else if (key == QStringLiteral("micA")) model = &m_micAEqBands;
        else if (key == QStringLiteral("micB")) model = &m_micBEqBands;
        else if (key == QStringLiteral("reverb")) model = &m_reverbEqBands;
        else if (key == QStringLiteral("echo")) model = &m_echoEqBands;
        else if (key == QStringLiteral("main")) model = &m_mainEqBands;
        else if (key == QStringLiteral("surround")) model = &m_surroundEqBands;
        else if (key == QStringLiteral("center")) model = &m_centerEqBands;
        else if (key == QStringLiteral("sub")) model = &m_subEqBands;

        QVariantList bands;
        for (int index = 0; index < section.bandCount; ++index) {
            const int offset = section.liveOffset + index * 5;
            const double frequency = u16At(memory, offset, 1000);
            const double q = qMax(1, static_cast<int>(byteAt(memory, offset + 2, 10))) / 10.0;
            const quint8 typeSign = byteAt(memory, offset + 3);
            const double gainMagnitude = byteAt(memory, offset + 4) / 10.0;
            const double gain = (typeSign & 0x80) ? -gainMagnitude : gainMagnitude;
            const QString typeName = bandTypeFromLive(typeSign);
            if (model && index < model->count())
                model->syncBand(index, frequency, gain, q, typeName);
            bands.append(QVariantMap{
                {QStringLiteral("index"), index + 1},
                {QStringLiteral("frequencyHz"), frequency},
                {QStringLiteral("q"), q},
                {QStringLiteral("gainDb"), gain},
                {QStringLiteral("type"), typeName},
            });
        }

        const double hpf = fileU16(memory, section.hpfFileOffset, 20);
        const double lpf = fileU16(memory, section.lpfFileOffset, 20000);
        QString hpType = QStringLiteral("HP Butter 12");
        QString lpType = QStringLiteral("LP Butter 12");
        if (model) {
            hpType = model->hpType();
            lpType = model->lpType();
            model->syncCrossover(hpf, lpf, hpType, lpType);
        }
        eqState.insert(key, QVariantMap{
            {QStringLiteral("bands"), bands},
            {QStringLiteral("hpfHz"), hpf},
            {QStringLiteral("lpfHz"), lpf},
            {QStringLiteral("hpType"), hpType},
            {QStringLiteral("lpType"), lpType},
        });
    }
    // Keep the canonical Music properties and graph model locked together.
    m_musicEqBands.syncCrossover(m_hpfHz, m_lpfHz, m_hpType, m_lpType);

    QStringList modeNames;
    for (int i = 0; i < 10; ++i)
        modeNames.append(fixedAscii(memory, 0x0290 + i * 0x10, 0x10));
    const QString activeName = fixedAscii(memory, 0x02C0, 0x10);
    int modeIndex = 4;
    for (int i = 0; i < modeNames.size(); ++i) {
        if (!activeName.isEmpty() && modeNames.at(i).compare(activeName, Qt::CaseInsensitive) == 0) {
            modeIndex = i + 1;
            break;
        }
    }

    QVariantMap system{
        {QStringLiteral("topMusicVol"), static_cast<int>(fileU8(memory, 0x0008))},
        {QStringLiteral("topMicVol"), static_cast<int>(fileU8(memory, 0x0009))},
        {QStringLiteral("topEffectVol"), static_cast<int>(fileU8(memory, 0x000A))},
        {QStringLiteral("musicInitVol"), static_cast<int>(fileU8(memory, 0x000B))},
        {QStringLiteral("musicMaxVol"), static_cast<int>(fileU8(memory, 0x000C))},
        {QStringLiteral("micInitVol"), static_cast<int>(fileU8(memory, 0x0012))},
        {QStringLiteral("micMaxVol"), static_cast<int>(fileU8(memory, 0x0013))},
        {QStringLiteral("effectInitLevel"), static_cast<int>(fileU8(memory, 0x001D))},
        {QStringLiteral("uDiskRecordVol"), static_cast<int>(fileU8(memory, 0x0095)) + 1},
        {QStringLiteral("usbRecordVol"), static_cast<int>(fileU8(memory, 0x0096)) + 1},
        {QStringLiteral("deviceModeIndex"), modeIndex},
        {QStringLiteral("deviceModeNames"), modeNames},
        {QStringLiteral("activeModeName"), activeName},
        {QStringLiteral("btName"), fixedAscii(memory, 0x0385, 0x13)},
        {QStringLiteral("bleName"), fixedAscii(memory, 0x0398, 0x13)},
    };

    QVariantMap mic{
        {QStringLiteral("micAVol"), static_cast<int>(fileU8(memory, 0x0014))},
        {QStringLiteral("micBVol"), static_cast<int>(fileU8(memory, 0x0015))},
        {QStringLiteral("fbxLevel"), qRound((fileU8(memory, 0x001B) + fileU8(memory, 0x001C)) / 2.0)},
        {QStringLiteral("noiseGateDb"), static_cast<int>(fileU8(memory, 0x0016)) - 81},
        {QStringLiteral("eqLink"), fileU8(memory, 0x0092) == 1},
        {QStringLiteral("hpfHz"), static_cast<int>(fileU16(memory, 0x0098))},
        {QStringLiteral("lpfHz"), static_cast<int>(fileU16(memory, 0x009A))},
    };
    mergeMap(mic, compState(memory, 0x0017));

    static const QStringList sourceNames{
        QStringLiteral("Input 1"), QStringLiteral("Input 2"), QStringLiteral("Bluetooth"),
        QStringLiteral("UDisk"), QStringLiteral("Digital")};
    const int sourceRaw = fileU8(memory, 0x000E);
    QVariantMap music{
        {QStringLiteral("sourceRaw"), sourceRaw},
        {QStringLiteral("source"), sourceRaw >= 0 && sourceRaw < sourceNames.size()
             ? sourceNames.at(sourceRaw) : QStringLiteral("Unknown %1").arg(sourceRaw)},
        {QStringLiteral("key"), static_cast<int>(fileU8(memory, 0x0011)) - 7},
        {QStringLiteral("input1GainDb"), static_cast<int>(fileU8(memory, 0x001E)) - 12},
        {QStringLiteral("input2GainDb"), static_cast<int>(fileU8(memory, 0x001F)) - 12},
        {QStringLiteral("btGainDb"), static_cast<int>(fileU8(memory, 0x0020)) - 12},
        {QStringLiteral("uDiskGainDb"), static_cast<int>(fileU8(memory, 0x0021)) - 12},
        {QStringLiteral("digitalGainDb"), static_cast<int>(fileU8(memory, 0x0022)) - 12},
    };

    QVariantMap mainOutput{
        {QStringLiteral("lVolDb"), outputDb(fileU8(memory, 0x0024))},
        {QStringLiteral("rVolDb"), outputDb(fileU8(memory, 0x0026))},
        {QStringLiteral("micDirect"), static_cast<int>(fileU8(memory, 0x0028))},
        {QStringLiteral("musicLevel"), static_cast<int>(fileU8(memory, 0x002A))},
        {QStringLiteral("reverbLevel"), static_cast<int>(fileU8(memory, 0x002C))},
        {QStringLiteral("echoLevel"), static_cast<int>(fileU8(memory, 0x002E))},
    };
    mergeMap(mainOutput, compState(memory, 0x0030));

    QVariantMap surroundOutput{
        {QStringLiteral("lVolDb"), outputDb(fileU8(memory, 0x0038))},
        {QStringLiteral("rVolDb"), outputDb(fileU8(memory, 0x003A))},
        {QStringLiteral("micDirect"), static_cast<int>(fileU8(memory, 0x003C))},
        {QStringLiteral("musicLevel"), static_cast<int>(fileU8(memory, 0x003E))},
        {QStringLiteral("reverbLevel"), static_cast<int>(fileU8(memory, 0x0040))},
        {QStringLiteral("echoLevel"), static_cast<int>(fileU8(memory, 0x0042))},
        {QStringLiteral("lDelayMs"), static_cast<int>(fileU16(memory, 0x00D8))},
        {QStringLiteral("rDelayMs"), static_cast<int>(fileU16(memory, 0x00DA))},
    };
    mergeMap(surroundOutput, compState(memory, 0x0044));

    QVariantMap centerOutput{
        {QStringLiteral("outputVolDb"), outputDb(fileU8(memory, 0x004C))},
        {QStringLiteral("micDirect"), static_cast<int>(fileU8(memory, 0x0050))},
        {QStringLiteral("musicLevel"), static_cast<int>(fileU8(memory, 0x0052))},
        {QStringLiteral("reverbLevel"), static_cast<int>(fileU8(memory, 0x0054))},
        {QStringLiteral("echoLevel"), static_cast<int>(fileU8(memory, 0x0056))},
    };
    mergeMap(centerOutput, compState(memory, 0x0058));

    QVariantMap subOutput{
        {QStringLiteral("outputVolDb"), outputDb(fileU8(memory, 0x0060))},
        {QStringLiteral("micDirect"), static_cast<int>(fileU8(memory, 0x0064))},
        {QStringLiteral("musicLevel"), static_cast<int>(fileU8(memory, 0x0066))},
        {QStringLiteral("reverbLevel"), static_cast<int>(fileU8(memory, 0x0068))},
        {QStringLiteral("echoLevel"), static_cast<int>(fileU8(memory, 0x006A))},
        {QStringLiteral("hpfHz"), static_cast<int>(fileU16(memory, 0x00B8))},
        {QStringLiteral("lpfHz"), static_cast<int>(fileU16(memory, 0x00BC))},
    };
    mergeMap(subOutput, compState(memory, 0x006C));

    QVariantMap reverb{
        {QStringLiteral("level"), static_cast<int>(fileU8(memory, 0x0074))},
        {QStringLiteral("hpfHz"), static_cast<int>(fileU16(memory, 0x00C0))},
        {QStringLiteral("lpfHz"), static_cast<int>(fileU16(memory, 0x00C2))},
        {QStringLiteral("decayMs"), static_cast<int>(fileU16(memory, 0x00C8))},
        {QStringLiteral("predelayMs"), static_cast<int>(fileU16(memory, 0x00CA))},
    };
    QVariantMap echo{
        {QStringLiteral("level"), static_cast<int>(fileU8(memory, 0x007B))},
        {QStringLiteral("repeat"), static_cast<int>(fileU8(memory, 0x007C))},
        {QStringLiteral("hpfHz"), static_cast<int>(fileU16(memory, 0x00C4))},
        {QStringLiteral("lpfHz"), static_cast<int>(fileU16(memory, 0x00C6))},
        {QStringLiteral("leftDelayMs"), static_cast<int>(fileU16(memory, 0x00CC))},
    };

    m_deviceState = {
        {QStringLiteral("memorySize"), memory.size()},
        {QStringLiteral("presetName"), activeName},
        {QStringLiteral("system"), system},
        {QStringLiteral("mic"), mic},
        {QStringLiteral("music"), music},
        {QStringLiteral("outputs"), QVariantMap{
            {QStringLiteral("main"), mainOutput},
            {QStringLiteral("surround"), surroundOutput},
            {QStringLiteral("center"), centerOutput},
            {QStringLiteral("sub"), subOutput},
        }},
        {QStringLiteral("effects"), QVariantMap{
            {QStringLiteral("reverb"), reverb},
            {QStringLiteral("echo"), echo},
        }},
        {QStringLiteral("eq"), eqState},
    };
    m_deviceStateReady = true;
    emit deviceStateChanged();
}

void StudioEngine::clearDeviceState()
{
    if (!m_deviceStateReady && m_deviceState.isEmpty())
        return;
    m_deviceState.clear();
    m_deviceStateReady = false;
    emit deviceStateChanged();
}

void StudioEngine::setMusicKey(int value)
{
    if (assign(m_musicKey, qBound(-7, value, 7), "music.key")) emit musicKeyChanged();
}

void StudioEngine::setNoiseGate(double value)
{
    if (assign(m_noiseGate, clampValue(value, -80.0, 0.0), "music.noiseGateDb")) emit noiseGateChanged();
}

void StudioEngine::setBass(double value)
{
    if (assign(m_bass, clampValue(value, -12.0, 12.0), "music.bassDb")) emit bassChanged();
}

void StudioEngine::setMid(double value)
{
    if (assign(m_mid, clampValue(value, -12.0, 12.0), "music.midDb")) emit midChanged();
}

void StudioEngine::setMidFreq(double value)
{
    if (assign(m_midFreq, clampValue(value, 80.0, 8000.0), "music.midFreqHz")) emit midFreqChanged();
}

void StudioEngine::setTreble(double value)
{
    if (assign(m_treble, clampValue(value, -12.0, 12.0), "music.trebleDb")) emit trebleChanged();
}

void StudioEngine::setHpfHz(double value)
{
    if (!assign(m_hpfHz, clampValue(value, 20.0, 20000.0), "eq.music.crossover.hpfHz")) return;
    syncMusicCrossoverModel();
    emit hpfHzChanged();
}

void StudioEngine::setLpfHz(double value)
{
    if (!assign(m_lpfHz, clampValue(value, 20.0, 20000.0), "eq.music.crossover.lpfHz")) return;
    syncMusicCrossoverModel();
    emit lpfHzChanged();
}

void StudioEngine::setHpType(const QString &value)
{
    if (!assign(m_hpType, value, "eq.music.crossover.hpType")) return;
    syncMusicCrossoverModel();
    emit hpTypeChanged();
}

void StudioEngine::setLpType(const QString &value)
{
    if (!assign(m_lpType, value, "eq.music.crossover.lpType")) return;
    syncMusicCrossoverModel();
    emit lpTypeChanged();
}

void StudioEngine::setInput1Gain(double value)
{
    if (assign(m_input1Gain, clampValue(value, -60.0, 10.0), "music.input1GainDb")) emit input1GainChanged();
}

void StudioEngine::setInput2Gain(double value)
{
    if (assign(m_input2Gain, clampValue(value, -60.0, 10.0), "music.input2GainDb")) emit input2GainChanged();
}

void StudioEngine::setBluetoothGain(double value)
{
    if (assign(m_bluetoothGain, clampValue(value, -60.0, 10.0), "music.bluetoothGainDb")) emit bluetoothGainChanged();
}

void StudioEngine::setUDiskGain(double value)
{
    if (assign(m_uDiskGain, clampValue(value, -60.0, 10.0), "music.uDiskGainDb")) emit uDiskGainChanged();
}

void StudioEngine::setDigitalGain(double value)
{
    if (assign(m_digitalGain, clampValue(value, -60.0, 10.0), "music.digitalGainDb")) emit digitalGainChanged();
}

void StudioEngine::setMasterMusic(double value)
{
    if (assign(m_masterMusic, clampValue(value, 0.0, 100.0), "system.topMusicVol")) emit masterMusicChanged();
}

void StudioEngine::setMasterMic(double value)
{
    if (assign(m_masterMic, clampValue(value, 0.0, 100.0), "system.topMicVol")) emit masterMicChanged();
}

void StudioEngine::setMasterFx(double value)
{
    if (assign(m_masterFx, clampValue(value, 0.0, 100.0), "system.topEffectVol")) emit masterFxChanged();
}

void StudioEngine::syncMusicCrossoverModel()
{
    m_musicEqBands.syncCrossover(m_hpfHz, m_lpfHz, m_hpType, m_lpType);
}
