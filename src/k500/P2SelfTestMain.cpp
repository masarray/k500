#include "K500PresetProtocol.h"

#include <QDebug>

int main()
{
    QString error;
    if (!K500PresetProtocol::selfTest(&error)) {
        qCritical().noquote() << "K500 P2 preset protocol self-test failed:" << error;
        return 1;
    }
    qInfo() << "K500 P2 preset protocol self-test passed";
    return 0;
}
