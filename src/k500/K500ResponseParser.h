#pragma once

#include <QByteArray>
#include <QList>
#include <QString>
#include <QtGlobal>

struct K500Response
{
    quint8 rsp = 0;
    QByteArray data;
    QByteArray raw;
    bool checksumOk = false;
};

class K500ResponseParser final
{
public:
    QList<K500Response> feed(const QByteArray &chunk);
    void reset() { m_buffer.clear(); }

    static bool selfTest(QString *error = nullptr);

private:
    QByteArray m_buffer;
};
