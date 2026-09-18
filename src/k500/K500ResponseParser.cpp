#include "K500ResponseParser.h"

#include "K500Frame.h"

namespace {
quint8 u8(char value)
{
    return static_cast<quint8>(static_cast<unsigned char>(value));
}

QByteArray responseFrame(quint8 rsp, const QByteArray &data)
{
    QByteArray raw;
    const int bodyLength = 1 + data.size();
    raw.reserve(1 + 2 + bodyLength + 1);
    raw.append(char(0x55));
    raw.append(char(bodyLength & 0xFF));
    raw.append(char((bodyLength >> 8) & 0xFF));
    raw.append(char(rsp));
    raw.append(data);

    quint8 sum = 0;
    for (qsizetype i = 1; i < raw.size(); ++i)
        sum = static_cast<quint8>(sum + u8(raw.at(i)));
    raw.append(char(static_cast<quint8>(0u - sum)));
    return raw;
}
}

QList<K500Response> K500ResponseParser::feed(const QByteArray &chunk)
{
    QList<K500Response> parsed;
    if (!chunk.isEmpty())
        m_buffer.append(chunk);

    while (m_buffer.size() >= 5) {
        const qsizetype header = m_buffer.indexOf(char(0x55));
        if (header < 0) {
            m_buffer.clear();
            break;
        }
        if (header > 0)
            m_buffer.remove(0, header);
        if (m_buffer.size() < 5)
            break;

        const int bodyLength = u8(m_buffer.at(1)) | (int(u8(m_buffer.at(2))) << 8);
        if (bodyLength <= 0 || bodyLength > 4096) {
            m_buffer.remove(0, 1);
            continue;
        }

        const int totalLength = 1 + 2 + bodyLength + 1;
        if (m_buffer.size() < totalLength)
            break;

        const QByteArray raw = m_buffer.left(totalLength);
        m_buffer.remove(0, totalLength);

        K500Response response;
        response.raw = raw;
        response.rsp = u8(raw.at(3));
        response.data = bodyLength > 1 ? raw.mid(4, bodyLength - 1) : QByteArray{};
        response.checksumOk = K500Frame::verify(raw);
        parsed.append(response);
    }

    return parsed;
}

bool K500ResponseParser::tryDecodePlaying(const K500Response &response, bool *playing)
{
    if (!playing || !response.checksumOk)
        return false;

    int statusIndex = -1;
    if (response.rsp == 0xE3)
        statusIndex = 2;   // capture: E3 00 05 [08|0C] 0D ...
    else if (response.rsp == 0xC0)
        statusIndex = 15;  // capture: C0 ... [08|0C] 0D AB 03 ...

    if (statusIndex < 0 || response.data.size() <= statusIndex)
        return false;

    const quint8 flags = u8(response.data.at(statusIndex));
    *playing = (flags & 0x04u) != 0;
    return true;
}

bool K500ResponseParser::selfTest(QString *error)
{
    const auto fail = [error](const QString &message) {
        if (error)
            *error = message;
        return false;
    };

    K500ResponseParser parser;
    const QByteArray status = responseFrame(0xE3, QByteArray(1, char(0x01)));
    const QByteArray scalars(0x40, char(0x19));

    // Exact physical K500 capture deltas from 2026-09-18:
    // STOPPED => status byte 0x08, PLAYING => status byte 0x0C.
    QByteArray stoppedStatusData = QByteArray::fromHex("0005080D666666666666660000");
    QByteArray playingStatusData = QByteArray::fromHex("00050C0D666666666666660000");
    K500Response stoppedStatus;
    stoppedStatus.rsp = 0xE3;
    stoppedStatus.data = stoppedStatusData;
    stoppedStatus.checksumOk = true;
    K500Response playingStatus = stoppedStatus;
    playingStatus.data = playingStatusData;
    bool playing = true;
    if (!tryDecodePlaying(stoppedStatus, &playing) || playing)
        return fail(QStringLiteral("captured STOPPED heartbeat playback decode mismatch"));
    if (!tryDecodePlaying(playingStatus, &playing) || !playing)
        return fail(QStringLiteral("captured PLAYING heartbeat playback decode mismatch"));

    QByteArray stoppedHandshakeData = QByteArray::fromHex("0017010205005A800100F501000000080DAB03CE0000");
    QByteArray playingHandshakeData = QByteArray::fromHex("0017010205005A800100F5010000000C0DAB03CE0000");
    K500Response stoppedHandshake;
    stoppedHandshake.rsp = 0xC0;
    stoppedHandshake.data = stoppedHandshakeData;
    stoppedHandshake.checksumOk = true;
    K500Response playingHandshake = stoppedHandshake;
    playingHandshake.data = playingHandshakeData;
    if (!tryDecodePlaying(stoppedHandshake, &playing) || playing)
        return fail(QStringLiteral("captured STOPPED handshake playback decode mismatch"));
    if (!tryDecodePlaying(playingHandshake, &playing) || !playing)
        return fail(QStringLiteral("captured PLAYING handshake playback decode mismatch"));
    const QByteArray read = responseFrame(0xBF, scalars);

    // Feed a split status frame, then padding and a complete second response.
    // The parser must recover on 0x55 boundaries.
    if (!parser.feed(status.left(2)).isEmpty())
        return fail(QStringLiteral("partial response emitted too early"));

    auto responses = parser.feed(status.mid(2) + QByteArray(8, char(0x00)) + read);
    if (responses.size() != 2)
        return fail(QStringLiteral("expected two parsed responses"));
    if (responses.at(0).rsp != 0xE3 || !responses.at(0).checksumOk)
        return fail(QStringLiteral("status response mismatch"));
    if (responses.at(1).rsp != 0xBF || !responses.at(1).checksumOk
        || responses.at(1).data.size() != 0x40)
        return fail(QStringLiteral("readback response mismatch"));

    // A 64-byte scalar response is 69 protocol bytes in total and therefore
    // crosses the K500's 64-byte USB HID payload boundary. Verify that the
    // stream parser preserves the incomplete first report until continuation.
    parser.reset();
    if (!parser.feed(read.left(64)).isEmpty())
        return fail(QStringLiteral("multi-report readback emitted before continuation"));
    responses = parser.feed(read.mid(64));
    if (responses.size() != 1 || responses.front().rsp != 0xBF
        || !responses.front().checksumOk || responses.front().data != scalars)
        return fail(QStringLiteral("multi-report scalar readback parse mismatch"));

    QByteArray corrupt = responseFrame(0xE3, {});
    corrupt[corrupt.size() - 1] = char(u8(corrupt.back()) ^ 0x01);
    responses = parser.feed(corrupt);
    if (responses.size() != 1 || responses.front().checksumOk)
        return fail(QStringLiteral("bad checksum was not detected"));

    return true;
}
