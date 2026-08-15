#include <QDebug>
#include <QFont>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>

#include "StudioEngine.h"
#include "k500/K500Controller.h"
#include "k500/K500DeviceManager.h"
#include "k500/K500Frame.h"
#include "k500/K500Protocol.h"
#include "k500/K500ResponseParser.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("SONKUPIK STUDIO Native UI"));
    app.setOrganizationName(QStringLiteral("MasArray"));

    // Match the current web product while using the native Windows variable font.
    QFont appFont(QStringLiteral("Segoe UI Variable Text"));
    appFont.setStyleStrategy(QFont::PreferAntialias);
    app.setFont(appFont);

    // Basic removes platform-heavy styling so every visible control is ours.
    QQuickStyle::setStyle(QStringLiteral("Basic"));

    StudioEngine studioEngine;
    K500Controller k500Controller;
    K500DeviceManager deviceManager(&k500Controller);

    // Native architecture boundary:
    // QML -> StudioEngine canonical path -> K500Controller -> K500DeviceManager
    // -> Windows COM / USB HID. LIVE remains disabled until the device manager
    // has completed protocol probe + scalar readback.
    QObject::connect(&studioEngine, &StudioEngine::stateEdited,
                     &k500Controller, &K500Controller::handleStateEdit);

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
        const QVariantMap band = studioEngine.musicEqBands()->get(2);
        const bool valid = qFuzzyCompare(studioEngine.bass(), 3.5)
                        && qFuzzyCompare(studioEngine.hpfHz(), 95.0)
                        && studioEngine.lpType() == QStringLiteral("LP LR 24")
                        && qFuzzyCompare(band.value(QStringLiteral("frequency")).toDouble(), 355.0)
                        && qFuzzyCompare(band.value(QStringLiteral("gain")).toDouble(), -11.1);
        return valid ? 0 : 2;
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
