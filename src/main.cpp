#include <QDebug>
#include <QFont>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QStringList>

#include "StudioEngine.h"
#include "k500/K500Controller.h"
#include "k500/K500DeviceManager.h"
#include "k500/K500Frame.h"
#include "k500/K500Protocol.h"
#include "k500/K500ResponseParser.h"

namespace {
const QString kUiFontFamily = QStringLiteral("Plus Jakarta Sans");

bool registerEmbeddedFonts()
{
    const QStringList fontResources = {
        QStringLiteral(":/fonts/PlusJakartaSans-Regular.ttf"),
        QStringLiteral(":/fonts/PlusJakartaSans-Medium.ttf"),
        QStringLiteral(":/fonts/PlusJakartaSans-SemiBold.ttf"),
        QStringLiteral(":/fonts/PlusJakartaSans-Bold.ttf"),
    };

    bool familyFound = false;
    for (const QString &resource : fontResources) {
        const int fontId = QFontDatabase::addApplicationFont(resource);
        if (fontId < 0) {
            qCritical().noquote() << "Failed to register embedded font:" << resource;
            return false;
        }
        const QStringList families = QFontDatabase::applicationFontFamilies(fontId);
        familyFound = familyFound || families.contains(kUiFontFamily);
    }
    return familyFound;
}

int liveOffsetForFileScalar(int fileOffset)
{
    if (fileOffset >= 0x0008 && fileOffset <= 0x0096)
        return fileOffset - 0x08;
    if (fileOffset >= 0x0098 && fileOffset <= 0x00EF)
        return fileOffset - 0x09;
    return -1;
}

void putFileU8(QByteArray &memory, int fileOffset, int value)
{
    const int offset = liveOffsetForFileScalar(fileOffset);
    if (offset >= 0 && offset < memory.size())
        memory[offset] = char(value & 0xFF);
}

void putFileU16(QByteArray &memory, int fileOffset, int value)
{
    const int offset = liveOffsetForFileScalar(fileOffset);
    if (offset >= 0 && offset + 1 < memory.size()) {
        memory[offset] = char(value & 0xFF);
        memory[offset + 1] = char((value >> 8) & 0xFF);
    }
}

void putFixedAscii(QByteArray &memory, int offset, int length, const QByteArray &text)
{
    if (offset < 0 || offset + length > memory.size())
        return;
    for (int i = 0; i < length; ++i)
        memory[offset + i] = char(0);
    const QByteArray clipped = text.left(length);
    for (int i = 0; i < clipped.size(); ++i)
        memory[offset + i] = clipped.at(i);
}

void putLiveEqBand(QByteArray &memory, int sectionOffset, int index,
                   int frequencyHz, int qTimes10, int typeSign, int gainMagnitudeTimes10)
{
    const int offset = sectionOffset + index * 5;
    if (offset < 0 || offset + 4 >= memory.size())
        return;
    memory[offset + 0] = char(frequencyHz & 0xFF);
    memory[offset + 1] = char((frequencyHz >> 8) & 0xFF);
    memory[offset + 2] = char(qTimes10 & 0xFF);
    memory[offset + 3] = char(typeSign & 0xFF);
    memory[offset + 4] = char(gainMagnitudeTimes10 & 0xFF);
}
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("SONKUPIK STUDIO Native UI"));
    app.setOrganizationName(QStringLiteral("MasArray"));

    if (!registerEmbeddedFonts()) {
        qCritical() << "Plus Jakarta Sans embedded font family is unavailable";
        return 5;
    }

    QFont appFont(kUiFontFamily);
    appFont.setStyleStrategy(QFont::PreferAntialias);
    app.setFont(appFont);

    if (app.arguments().contains(QStringLiteral("--font-self-test"))) {
        const bool ok = QFontDatabase::families().contains(kUiFontFamily)
                     && app.font().family() == kUiFontFamily;
        if (!ok) {
            qCritical() << "Embedded Plus Jakarta Sans self-test failed";
            return 6;
        }
        qInfo() << "Embedded Plus Jakarta Sans self-test passed";
        return 0;
    }

    QQuickStyle::setStyle(QStringLiteral("Basic"));

    StudioEngine studioEngine;
    K500Controller k500Controller;
    K500DeviceManager deviceManager(&k500Controller);

    // P0_NATIVE_ARCHITECTURE_V1
    // QML -> StudioEngine canonical path -> K500Controller -> K500DeviceManager
    // -> native transport. Reverse sync is the exact donor order:
    // 0x1C -> 0x3F -> 0x40 full 0x03AB active memory -> StudioEngine -> LIVE ON.
    QObject::connect(&studioEngine, &StudioEngine::stateEdited,
                     &k500Controller, &K500Controller::handleStateEdit);
    QObject::connect(&deviceManager, &K500DeviceManager::activeMemoryReady,
                     &studioEngine, &StudioEngine::hydrateFromDeviceMemory);

    if (app.arguments().contains(QStringLiteral("--trace-k500"))) {
        QObject::connect(&deviceManager, &K500DeviceManager::logLine,
                         [](const QString &direction, const QString &label, const QString &hex) {
            qInfo().noquote() << "K500" << direction << label << hex;
        });
        QObject::connect(&k500Controller, &K500Controller::writeDeferred,
                         [](const QString &path, const QString &reason) {
            qInfo().noquote() << "K500 deferred" << path << "-" << reason;
        });
        QObject::connect(&k500Controller, &K500Controller::unsupportedPath,
                         [](const QString &path) {
            qInfo().noquote() << "K500 unmapped" << path;
        });
    }

    if (app.arguments().contains(QStringLiteral("--protocol-self-test"))) {
        QString error;
        if (!K500Protocol::selfTest(&error)) {
            qCritical().noquote() << "K500 protocol self-test failed:" << error;
            return 3;
        }
        if (!K500ResponseParser::selfTest(&error)) {
            qCritical().noquote() << "K500 RX parser self-test failed:" << error;
            return 4;
        }
        qInfo() << "K500 protocol + RX parser self-test passed";
        return 0;
    }

    if (app.arguments().contains(QStringLiteral("--engine-self-test"))) {
        studioEngine.setBass(3.5);
        studioEngine.setHpfHz(95.0);
        studioEngine.setLpType(QStringLiteral("LP LR 24"));
        studioEngine.musicEqBands()->setBand(2, 355.0, -11.1, 1.0);
        const QVariantMap editedBand = studioEngine.musicEqBands()->get(2);
        const bool editValid = qFuzzyCompare(studioEngine.bass(), 3.5)
                            && qFuzzyCompare(studioEngine.hpfHz(), 95.0)
                            && studioEngine.lpType() == QStringLiteral("LP LR 24")
                            && qFuzzyCompare(editedBand.value(QStringLiteral("frequency")).toDouble(), 355.0)
                            && qFuzzyCompare(editedBand.value(QStringLiteral("gain")).toDouble(), -11.1);
        if (!editValid)
            return 2;

        // P0_FULL_MEMORY_HYDRATION_V1
        // Synthetic active-memory snapshot verifies the donor split-offset map,
        // all major state families, compact EQ decode and the critical rule that
        // hydration is read-only (zero stateEdited/echo writes).
        QByteArray memory(0x03AB, char(0));

        // System / top-level state.
        putFileU8(memory, 0x0008, 61); // master music
        putFileU8(memory, 0x0009, 57); // master mic
        putFileU8(memory, 0x000A, 49); // master effect
        putFileU8(memory, 0x000B, 25); // music init
        putFileU8(memory, 0x000C, 84); // music max
        putFileU8(memory, 0x0012, 26); // mic init
        putFileU8(memory, 0x0013, 82); // mic max
        putFileU8(memory, 0x001D, 27); // effect init
        putFileU8(memory, 0x0095, 8);  // U-Disk record -> UI 9
        putFileU8(memory, 0x0096, 10); // USB record -> UI 11

        // Music.
        putFileU8(memory, 0x000E, 4);  // Digital
        putFileU8(memory, 0x0011, 10); // key +3
        putFileU8(memory, 0x001E, 15); // input1 +3 dB
        putFileU8(memory, 0x001F, 11); // input2 -1 dB
        putFileU8(memory, 0x0020, 17); // BT +5 dB
        putFileU8(memory, 0x0021, 9);  // UDisk -3 dB
        putFileU8(memory, 0x0022, 8);  // Digital -4 dB
        putFileU16(memory, 0x009C, 80);    // Music HPF
        putFileU16(memory, 0x009E, 18000); // Music LPF

        // Mic state and dynamics.
        putFileU8(memory, 0x0014, 96);
        putFileU8(memory, 0x0015, 94);
        putFileU8(memory, 0x0016, 70); // noise gate = -11 dB
        putFileU8(memory, 0x0017, 38); // compressor TH = -12 dB
        putFileU8(memory, 0x0018, 3);
        putFileU8(memory, 0x0019, 10);
        putFileU8(memory, 0x001A, 2);  // 0.2 sec
        putFileU8(memory, 0x001B, 5);
        putFileU8(memory, 0x001C, 7);  // shared UI average = 6
        putFileU8(memory, 0x0092, 1);  // EQ link
        putFileU16(memory, 0x0098, 90);
        putFileU16(memory, 0x009A, 16000);

        // Main output.
        putFileU8(memory, 0x0024, 99); // +12 dB
        putFileU8(memory, 0x0026, 95); // +10 dB
        putFileU8(memory, 0x0028, 91);
        putFileU8(memory, 0x002A, 87);
        putFileU8(memory, 0x002C, 83);
        putFileU8(memory, 0x002E, 79);
        putFileU8(memory, 0x0030, 36); // -14 dB
        putFileU8(memory, 0x0031, 4);
        putFileU8(memory, 0x0032, 12);
        putFileU8(memory, 0x0033, 3);  // 0.3 sec
        putFileU16(memory, 0x00A0, 45);
        putFileU16(memory, 0x00A4, 19000);

        // Surround output + delay.
        putFileU8(memory, 0x0038, 93); // +9 dB
        putFileU8(memory, 0x003A, 91); // +8 dB
        putFileU8(memory, 0x003C, 78);
        putFileU8(memory, 0x003E, 76);
        putFileU8(memory, 0x0040, 74);
        putFileU8(memory, 0x0042, 72);
        putFileU8(memory, 0x0044, 35); // -15 dB
        putFileU8(memory, 0x0045, 5);
        putFileU8(memory, 0x0046, 13);
        putFileU8(memory, 0x0047, 4);
        putFileU16(memory, 0x00D8, 17);
        putFileU16(memory, 0x00DA, 19);

        // Center output.
        putFileU8(memory, 0x004C, 89); // +7 dB
        putFileU8(memory, 0x0050, 71);
        putFileU8(memory, 0x0052, 69);
        putFileU8(memory, 0x0054, 67);
        putFileU8(memory, 0x0056, 65);
        putFileU8(memory, 0x0058, 34); // -16 dB
        putFileU8(memory, 0x0059, 6);
        putFileU8(memory, 0x005A, 14);
        putFileU8(memory, 0x005B, 5);

        // Sub output + crossover.
        putFileU8(memory, 0x0060, 87); // +6 dB
        putFileU8(memory, 0x0064, 64);
        putFileU8(memory, 0x0066, 62);
        putFileU8(memory, 0x0068, 60);
        putFileU8(memory, 0x006A, 58);
        putFileU8(memory, 0x006C, 33); // -17 dB
        putFileU8(memory, 0x006D, 7);
        putFileU8(memory, 0x006E, 15);
        putFileU8(memory, 0x006F, 6);
        putFileU16(memory, 0x00B8, 42);
        putFileU16(memory, 0x00BC, 96);

        // Effects.
        putFileU8(memory, 0x0074, 55);
        putFileU8(memory, 0x007B, 44);
        putFileU8(memory, 0x007C, 6);
        putFileU16(memory, 0x00C0, 220);
        putFileU16(memory, 0x00C2, 12000);
        putFileU16(memory, 0x00C4, 700);
        putFileU16(memory, 0x00C6, 4400);
        putFileU16(memory, 0x00C8, 1850);
        putFileU16(memory, 0x00CA, 28);
        putFileU16(memory, 0x00CC, 236);

        // Compact live EQ representations from three different sections.
        putLiveEqBand(memory, 0x014B, 2, 355, 10, 0x80, 111); // Music B3 Bell -11.1
        putLiveEqBand(memory, 0x00E7, 0, 125, 14, 0x10, 25);  // Mic A B1 low-shelf +2.5
        putLiveEqBand(memory, 0x0218, 4, 96, 20, 0xA0, 30);   // Sub B5 high-shelf -3.0

        const QStringList names{
            QStringLiteral("ARTIST GEN3 ARI"), QStringLiteral("PODCAST REBORN"),
            QStringLiteral("DANGDUT GEN3"), QStringLiteral("KARAOKE ARTIST"),
            QStringLiteral("AKUSTIK GEN3"), QStringLiteral("IMAM QORI GEN3"),
            QStringLiteral("JAZZ GEN3"), QStringLiteral("ROCK GEN3"),
            QStringLiteral("MC CERAMAH"), QStringLiteral("ADZAN MEKAH")};
        for (int i = 0; i < names.size(); ++i)
            putFixedAscii(memory, 0x0290 + i * 0x10, 0x10, names.at(i).toLatin1());
        putFixedAscii(memory, 0x02C0, 0x10, QByteArray("KARAOKE ARTIST"));
        putFixedAscii(memory, 0x0385, 0x13, QByteArray("KTV_BT_TEST"));
        putFixedAscii(memory, 0x0398, 0x13, QByteArray("KTV_BLE_TEST"));

        int hydrationEdits = 0;
        QObject::connect(&studioEngine, &StudioEngine::stateEdited,
                         [&hydrationEdits](const QString &, const QVariant &) { ++hydrationEdits; });
        studioEngine.hydrateFromDeviceMemory(memory);

        const QVariantMap state = studioEngine.deviceState();
        const QVariantMap system = state.value(QStringLiteral("system")).toMap();
        const QVariantMap mic = state.value(QStringLiteral("mic")).toMap();
        const QVariantMap music = state.value(QStringLiteral("music")).toMap();
        const QVariantMap outputs = state.value(QStringLiteral("outputs")).toMap();
        const QVariantMap mainOutput = outputs.value(QStringLiteral("main")).toMap();
        const QVariantMap surroundOutput = outputs.value(QStringLiteral("surround")).toMap();
        const QVariantMap centerOutput = outputs.value(QStringLiteral("center")).toMap();
        const QVariantMap subOutput = outputs.value(QStringLiteral("sub")).toMap();
        const QVariantMap effects = state.value(QStringLiteral("effects")).toMap();
        const QVariantMap reverb = effects.value(QStringLiteral("reverb")).toMap();
        const QVariantMap echo = effects.value(QStringLiteral("echo")).toMap();
        const QVariantMap eq = state.value(QStringLiteral("eq")).toMap();
        const QVariantMap mainEq = eq.value(QStringLiteral("main")).toMap();
        const QVariantMap hydratedMusicBand = studioEngine.musicEqBands()->get(2);
        const QVariantMap hydratedMicBand = studioEngine.micAEqBands()->get(0);
        const QVariantMap hydratedSubBand = studioEngine.subEqBands()->get(4);

        const bool hydrationValid = studioEngine.deviceStateReady()
            && state.value(QStringLiteral("memorySize")).toInt() == 0x03AB
            && state.value(QStringLiteral("presetName")).toString() == QStringLiteral("KARAOKE ARTIST")
            && qFuzzyCompare(studioEngine.masterMusic(), 61.0)
            && qFuzzyCompare(studioEngine.masterMic(), 57.0)
            && qFuzzyCompare(studioEngine.masterFx(), 49.0)
            && studioEngine.musicKey() == 3
            && qFuzzyCompare(studioEngine.input1Gain(), 3.0)
            && qFuzzyCompare(studioEngine.input2Gain(), -1.0)
            && qFuzzyCompare(studioEngine.bluetoothGain(), 5.0)
            && qFuzzyCompare(studioEngine.uDiskGain(), -3.0)
            && qFuzzyCompare(studioEngine.digitalGain(), -4.0)
            && qFuzzyCompare(studioEngine.hpfHz(), 80.0)
            && qFuzzyCompare(studioEngine.lpfHz(), 18000.0)
            && system.value(QStringLiteral("musicInitVol")).toInt() == 25
            && system.value(QStringLiteral("musicMaxVol")).toInt() == 84
            && system.value(QStringLiteral("micInitVol")).toInt() == 26
            && system.value(QStringLiteral("micMaxVol")).toInt() == 82
            && system.value(QStringLiteral("effectInitLevel")).toInt() == 27
            && system.value(QStringLiteral("uDiskRecordVol")).toInt() == 9
            && system.value(QStringLiteral("usbRecordVol")).toInt() == 11
            && system.value(QStringLiteral("deviceModeIndex")).toInt() == 4
            && system.value(QStringLiteral("activeModeName")).toString() == QStringLiteral("KARAOKE ARTIST")
            && system.value(QStringLiteral("btName")).toString() == QStringLiteral("KTV_BT_TEST")
            && system.value(QStringLiteral("bleName")).toString() == QStringLiteral("KTV_BLE_TEST")
            && mic.value(QStringLiteral("micAVol")).toInt() == 96
            && mic.value(QStringLiteral("micBVol")).toInt() == 94
            && mic.value(QStringLiteral("fbxLevel")).toInt() == 6
            && mic.value(QStringLiteral("noiseGateDb")).toInt() == -11
            && mic.value(QStringLiteral("eqLink")).toBool()
            && mic.value(QStringLiteral("compThresholdDb")).toInt() == -12
            && mic.value(QStringLiteral("compRatio")).toInt() == 3
            && qFuzzyCompare(mic.value(QStringLiteral("releaseSec")).toDouble(), 0.2)
            && music.value(QStringLiteral("source")).toString() == QStringLiteral("Digital")
            && qFuzzyCompare(mainOutput.value(QStringLiteral("lVolDb")).toDouble(), 12.0)
            && qFuzzyCompare(mainOutput.value(QStringLiteral("rVolDb")).toDouble(), 10.0)
            && mainOutput.value(QStringLiteral("compThresholdDb")).toInt() == -14
            && qFuzzyCompare(surroundOutput.value(QStringLiteral("lVolDb")).toDouble(), 9.0)
            && surroundOutput.value(QStringLiteral("lDelayMs")).toInt() == 17
            && surroundOutput.value(QStringLiteral("rDelayMs")).toInt() == 19
            && qFuzzyCompare(centerOutput.value(QStringLiteral("outputVolDb")).toDouble(), 7.0)
            && qFuzzyCompare(subOutput.value(QStringLiteral("outputVolDb")).toDouble(), 6.0)
            && subOutput.value(QStringLiteral("hpfHz")).toInt() == 42
            && subOutput.value(QStringLiteral("lpfHz")).toInt() == 96
            && reverb.value(QStringLiteral("level")).toInt() == 55
            && reverb.value(QStringLiteral("decayMs")).toInt() == 1850
            && reverb.value(QStringLiteral("predelayMs")).toInt() == 28
            && echo.value(QStringLiteral("level")).toInt() == 44
            && echo.value(QStringLiteral("repeat")).toInt() == 6
            && echo.value(QStringLiteral("leftDelayMs")).toInt() == 236
            && qFuzzyCompare(hydratedMusicBand.value(QStringLiteral("frequency")).toDouble(), 355.0)
            && qFuzzyCompare(hydratedMusicBand.value(QStringLiteral("gain")).toDouble(), -11.1)
            && qFuzzyCompare(hydratedMicBand.value(QStringLiteral("frequency")).toDouble(), 125.0)
            && qFuzzyCompare(hydratedMicBand.value(QStringLiteral("gain")).toDouble(), 2.5)
            && hydratedMicBand.value(QStringLiteral("typeName")).toString() == QStringLiteral("LOW SHELF")
            && qFuzzyCompare(hydratedSubBand.value(QStringLiteral("frequency")).toDouble(), 96.0)
            && qFuzzyCompare(hydratedSubBand.value(QStringLiteral("gain")).toDouble(), -3.0)
            && hydratedSubBand.value(QStringLiteral("typeName")).toString() == QStringLiteral("HIGH SHELF")
            && qFuzzyCompare(mainEq.value(QStringLiteral("hpfHz")).toDouble(), 45.0)
            && qFuzzyCompare(mainEq.value(QStringLiteral("lpfHz")).toDouble(), 19000.0)
            && hydrationEdits == 0;
        return hydrationValid ? 0 : 7;
    }

    QQmlApplicationEngine engine;
    engine.setInitialProperties({
        {QStringLiteral("studioEngine"), QVariant::fromValue(&studioEngine)},
        {QStringLiteral("deviceManager"), QVariant::fromValue(&deviceManager)},
    });
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    engine.loadFromModule(QStringLiteral("SonkupikStudio"), QStringLiteral("Main"));
    return app.exec();
}
