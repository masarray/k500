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

    // PLAYER_STATUS_CAPTURED_V1
    // Native captures show the same transport-status byte in RSP 0xE3 and
    // RSP 0xC0. Bit 0x04 is clear while stopped/paused (0x08) and set while
    // music is actively playing (0x0C).
    static bool tryDecodePlaying(const K500Response &response, bool *playing);
    // USE_INIT_CONNECT_CAPTURED_V1
    // Physical connect captures prove C0 data[7] bit 0x04 is the authoritative
    // Use Init Volume state: clear=OFF (0x80), set=ON (0x84).
    static bool tryDecodeUseInitVolume(const K500Response &response, bool *enabled);
    // MUTE_CONNECT_CAPTURED_V1
    // Physical connect captures prove C0 data[7] bit 0x02 is Mute:
    // clear=unmuted (0x84), set=muted (0x86).
    static bool tryDecodeMuted(const K500Response &response, bool *muted);
    static bool selfTest(QString *error = nullptr);

private:
    QByteArray m_buffer;
};
