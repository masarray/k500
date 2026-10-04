#include "K500Protocol.h"

#include <QCoreApplication>
#include <QTextStream>

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QString error;
    if (!K500Protocol::selfTest(&error)) {
        QTextStream(stderr) << "K500 protocol self-test FAILED: " << error << "\n";
        return 1;
    }
    QTextStream(stdout) << "K500 protocol self-test PASS\n";
    return 0;
}
