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

    // Native architecture boundary:
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

        // Synthetic active-memory snapshot verifies the donor split-offset map,
        // compact EQ decode and the critical rule that hydration is read-only.
        QByteArray memory(0x03AB, char(0));
        putFileU8(memory, 0x0008, 61); // master music
        putFileU8(memory, 0x0009, 57); // master mic
        putFileU8(memory, 0x000A, 49); // master effect
        putFileU8(memory, 0x0011, 10); // key +3
        putFileU8(memory, 0x001E, 15); // input1 +3 dB
        putFileU8(memory, 0x001F, 11); // input2 -1 dB
        putFileU8(memory, 0x0020, 17); // BT +5 dB
        putFileU8(memory, 0x0021, 9);  // UDisk -3 dB
        putFileU8(memory, 0x0022, 8);  // Digital -4 dB
        putFileU8(memory, 0x0024, 99); // Main L +12 dB
        putFileU8(memory, 0x0026, 95); // Main R +10 dB
        putFileU8(memory, 0x0028, 91);
        putFileU8(memory, 0x002A, 87);
        putFileU8(memory, 0x002C, 83);
        putFileU8(memory, 0x002E, 79);
        putFileU16(memory, 0x009C, 80);    // Music HPF
        putFileU16(memory, 0x009E, 18000); // Music LPF
        putFileU16(memory, 0x00A0, 45);    // Main HPF
        putFileU16(memory, 0x00A4, 19000); // Main LPF

        const int musicEq = 0x014B + 2 * 5;
        memory[musicEq + 0] = char(355 & 0xFF);
        memory[musicEq + 1] = char((355 >> 8) & 0xFF);
        memory[musicEq + 2] = char(10);       // Q 1.0
        memory[musicEq + 3] = char(0x80);     // Bell, negative
        memory[musicEq + 4] = char(111);      // -11.1 dB

        const QStringList names{
            QStringLiteral("ARTIST GEN3 ARI"), QStringLiteral("PODCAST REBORN"),
            QStringLiteral("DANGDUT GEN3"), QStringLiteral("KARAOKE ARTIST"),
            QStringLiteral("AKUSTIK GEN3"), QStringLiteral("IMAM QORI GEN3"),
            QStringLiteral("JAZZ GEN3"), QStringLiteral("ROCK GEN3"),
            QStringLiteral("MC CERAMAH"), QStringLiteral("ADZAN MEKAH")};
        for (int i = 0; i < names.size(); ++i)
            putFixedAscii(memory, 0x0290 + i * 0x10, 0x10, names.at(i).toLatin1());
        putFixedAscii(memory, 0x0385, 0x13, QByteArray("KTV_BT_TEST"));
        putFixedAscii(memory, 0x0398, 0x13, QByteArray("KTV_BLE_TEST"));

        int hydrationEdits = 0;
        QObject::connect(&studioEngine, &StudioEngine::stateEdited,
                         [&hydrationEdits](const QString &, const QVariant &) { ++hydrationEdits; });
        studioEngine.hydrateFromDeviceMemory(memory);

        const QVariantMap state = studioEngine.deviceState();
        const QVariantMap system = state.value(QStringLiteral("system")).toMap();
        const QVariantMap outputs = state.value(QStringLiteral("outputs")).toMap();
        const QVariantMap mainOutput = outputs.value(QStringLiteral("main")).toMap();
        const QVariantMap hydratedBand = studioEngine.musicEqBands()->get(2);
        const bool hydrationValid = studioEngine.deviceStateReady()
            && state.value(QStringLiteral("memorySize")).toInt() == 0x03AB
            && qFuzzyCompare(studioEngine.masterMusic(), 61.0)
            && studioEngine.musicKey() == 3
            && qFuzzyCompare(studioEngine.input1Gain(), 3.0)
            && qFuzzyCompare(studioEngine.hpfHz(), 80.0)
            && qFuzzyCompare(studioEngine.lpfHz(), 18000.0)
            && qFuzzyCompare(hydratedBand.value(QStringLiteral("frequency")).toDouble(), 355.0)
            && qFuzzyCompare(hydratedBand.value(QStringLiteral("gain")).toDouble(), -11.1)
            && system.value(QStringLiteral("deviceModeIndex")).toInt() == 4
            && system.value(QStringLiteral("btName")).toString() == QStringLiteral("KTV_BT_TEST")
            && qFuzzyCompare(mainOutput.value(QStringLiteral("lVolDb")).toDouble(), 12.0)
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
