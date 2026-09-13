#include "AppUpdateManager.h"

#include <QCoreApplication>
#include <QDebug>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("SonKuPik K500 Update Self-Test"));
    app.setOrganizationName(QStringLiteral("MasArray"));

    QString error;
    if (!AppUpdateManager::selfTest(&error)) {
        qCritical().noquote() << "Updater metadata self-test failed:" << error;
        return 1;
    }

    qInfo() << "Updater metadata self-test passed";
    return 0;
}
