#include "RuntimeMetrics.h"
#include "StudioEngine.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QSysInfo>
#include <QTextStream>
#include <QtGlobal>

#include <algorithm>

namespace {
constexpr int WarmupEditRounds = 80;
constexpr int WarmupLifecycleCycles = 20;
constexpr int MeasuredEditRounds = 700;
constexpr int MeasuredLifecycleCycles = 160;
constexpr qint64 MaxSteadyPrivateGrowthBytes = 8LL * 1024LL * 1024LL;
constexpr qint64 MaxSteadyHandleGrowth = 2;
constexpr qint64 MaxSteadyThreadGrowth = 1;

QList<EqBandModel *> eqModels(StudioEngine &engine)
{
    return {
        engine.musicEqBands(),
        engine.micAEqBands(),
        engine.micBEqBands(),
        engine.reverbEqBands(),
        engine.echoEqBands(),
        engine.mainEqBands(),
        engine.surroundEqBands(),
        engine.centerEqBands(),
        engine.subEqBands(),
    };
}

quint64 exerciseEngine(StudioEngine &engine, int rounds)
{
    const QList<EqBandModel *> models = eqModels(engine);
    quint64 checksum = 0;

    for (int round = 0; round < rounds; ++round) {
        for (int modelIndex = 0; modelIndex < models.size(); ++modelIndex) {
            EqBandModel *model = models.at(modelIndex);
            for (int band = 0; band < model->count(); ++band) {
                const int seed = round * 37 + modelIndex * 19 + band * 11;
                const double frequency = 20.0 + static_cast<double>(seed % 19980);
                const double gain = static_cast<double>((seed % 97) - 48) * 0.5;
                const double q = 0.2 + static_cast<double>(seed % 120) * 0.1;
                model->setBand(band, frequency, gain, q);

                if ((seed % 29) == 0)
                    model->setBandType(band, (seed & 1) ? QStringLiteral("LOW SHELF")
                                                       : QStringLiteral("HIGH SHELF"));
                else if ((seed % 31) == 0)
                    model->setBandType(band, QStringLiteral("BELL"));

                const QVariantMap state = model->get(band);
                checksum += static_cast<quint64>(state.value(QStringLiteral("frequency")).toDouble());
                checksum += static_cast<quint64>((state.value(QStringLiteral("gain")).toDouble() + 24.0) * 10.0);
                checksum += static_cast<quint64>(state.value(QStringLiteral("q")).toDouble() * 10.0);
            }

            if ((round % 13) == 0) {
                model->setHpfHz(20.0 + ((round + modelIndex) % 180) * 5.0);
                model->setLpfHz(12000.0 + ((round + modelIndex) % 80) * 100.0);
            }
        }

        engine.setBass(static_cast<double>((round % 49) - 24) * 0.5);
        engine.setMid(static_cast<double>((round % 41) - 20) * 0.5);
        engine.setTreble(static_cast<double>((round % 45) - 22) * 0.5);
        engine.setMusicKey((round % 15) - 7);
    }

    return checksum;
}

quint64 exerciseLifecycle(int cycles)
{
    quint64 checksum = 0;
    for (int cycle = 0; cycle < cycles; ++cycle) {
        StudioEngine engine;
        checksum ^= exerciseEngine(engine, 2);
        checksum += static_cast<quint64>(engine.musicEqBands()->count());
    }
    return checksum;
}

QJsonObject snapshotJson(const RuntimeHealthSnapshot &snapshot)
{
    return {
        {QStringLiteral("valid"), snapshot.valid},
        {QStringLiteral("workingSetBytes"), snapshot.workingSetBytes},
        {QStringLiteral("privateBytes"), snapshot.privateBytes},
        {QStringLiteral("peakWorkingSetBytes"), snapshot.peakWorkingSetBytes},
        {QStringLiteral("handleCount"), static_cast<qint64>(snapshot.handleCount)},
        {QStringLiteral("threadCount"), static_cast<qint64>(snapshot.threadCount)},
    };
}

qint64 positiveGrowth(qint64 after, qint64 before)
{
    if (after < 0 || before < 0)
        return -1;
    return std::max<qint64>(0, after - before);
}

struct PhaseResult
{
    qint64 elapsedNs = 0;
    quint64 checksum = 0;
};

PhaseResult runMeasuredPhase(StudioEngine &engine)
{
    QElapsedTimer timer;
    timer.start();
    quint64 checksum = exerciseEngine(engine, MeasuredEditRounds);
    checksum ^= exerciseLifecycle(MeasuredLifecycleCycles);
    QCoreApplication::processEvents();
    return {timer.nsecsElapsed(), checksum};
}
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("K500 P0 Runtime Telemetry"));

    StudioEngine engine;

    // P0_RUNTIME_TELEMETRY_V1
    // Warm the Qt/container allocators before establishing the resource baseline.
    // The pass/fail gate compares two equivalent measured phases so normal cache
    // initialization is not mistaken for a leak.
    quint64 checksum = exerciseEngine(engine, WarmupEditRounds);
    checksum ^= exerciseLifecycle(WarmupLifecycleCycles);
    QCoreApplication::processEvents();

    const RuntimeHealthSnapshot warm = RuntimeMetrics::capture();
    const PhaseResult phaseA = runMeasuredPhase(engine);
    const RuntimeHealthSnapshot afterA = RuntimeMetrics::capture();
    const PhaseResult phaseB = runMeasuredPhase(engine);
    const RuntimeHealthSnapshot afterB = RuntimeMetrics::capture();
    checksum ^= phaseA.checksum;
    checksum ^= phaseB.checksum;

    const qint64 privateGrowth = positiveGrowth(afterB.privateBytes, afterA.privateBytes);
    const qint64 workingSetGrowth = positiveGrowth(afterB.workingSetBytes, afterA.workingSetBytes);
    const qint64 handleGrowth = static_cast<qint64>(afterB.handleCount) - static_cast<qint64>(afterA.handleCount);
    const qint64 threadGrowth = static_cast<qint64>(afterB.threadCount) - static_cast<qint64>(afterA.threadCount);

#ifdef Q_OS_WIN
    const bool metricsValid = warm.valid && afterA.valid && afterB.valid;
#else
    const bool metricsValid = true;
#endif
    const bool resourceStable = !afterB.valid
        || ((privateGrowth >= 0 && privateGrowth <= MaxSteadyPrivateGrowthBytes)
            && handleGrowth <= MaxSteadyHandleGrowth
            && threadGrowth <= MaxSteadyThreadGrowth);
    const bool passed = metricsValid && resourceStable;

    QJsonObject limits{
        {QStringLiteral("steadyPrivateGrowthBytes"), MaxSteadyPrivateGrowthBytes},
        {QStringLiteral("steadyHandleGrowth"), MaxSteadyHandleGrowth},
        {QStringLiteral("steadyThreadGrowth"), MaxSteadyThreadGrowth},
    };
    QJsonObject steadyDelta{
        {QStringLiteral("privateBytes"), privateGrowth},
        {QStringLiteral("workingSetBytes"), workingSetGrowth},
        {QStringLiteral("handles"), handleGrowth},
        {QStringLiteral("threads"), threadGrowth},
    };
    QJsonObject timing{
        {QStringLiteral("phaseAMs"), static_cast<double>(phaseA.elapsedNs) / 1000000.0},
        {QStringLiteral("phaseBMs"), static_cast<double>(phaseB.elapsedNs) / 1000000.0},
    };
    QJsonObject workload{
        {QStringLiteral("warmupEditRounds"), WarmupEditRounds},
        {QStringLiteral("warmupLifecycleCycles"), WarmupLifecycleCycles},
        {QStringLiteral("measuredEditRoundsPerPhase"), MeasuredEditRounds},
        {QStringLiteral("measuredLifecycleCyclesPerPhase"), MeasuredLifecycleCycles},
    };

    QJsonObject root{
        {QStringLiteral("schema"), QStringLiteral("sonkupik-k500-runtime-telemetry-v1")},
        {QStringLiteral("platform"), QSysInfo::prettyProductName()},
        {QStringLiteral("cpuArchitecture"), QSysInfo::currentCpuArchitecture()},
        {QStringLiteral("qtVersion"), QString::fromLatin1(qVersion())},
        {QStringLiteral("workload"), workload},
        {QStringLiteral("timing"), timing},
        {QStringLiteral("warm"), snapshotJson(warm)},
        {QStringLiteral("afterPhaseA"), snapshotJson(afterA)},
        {QStringLiteral("afterPhaseB"), snapshotJson(afterB)},
        {QStringLiteral("steadyDelta"), steadyDelta},
        {QStringLiteral("limits"), limits},
        {QStringLiteral("checksum"), QString::number(checksum)},
        {QStringLiteral("pass"), passed},
    };

    QTextStream(stdout) << QJsonDocument(root).toJson(QJsonDocument::Compact) << Qt::endl;
    return passed ? 0 : 9;
}
