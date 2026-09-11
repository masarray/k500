#include "K500Frame.h"
#include "K500ResponseParser.h"

#include <QByteArray>
#include <QDebug>
#include <QString>

namespace {
constexpr int FuzzIterations = 60000;
constexpr quint32 InitialSeed = 0x4B353030u; // "K500"

quint8 u8(char value)
{
    return static_cast<quint8>(static_cast<unsigned char>(value));
}

class XorShift32 final
{
public:
    explicit XorShift32(quint32 seed) : m_state(seed ? seed : 1u) {}

    quint32 next()
    {
        quint32 x = m_state;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        m_state = x;
        return x;
    }

    int bounded(int exclusiveUpper)
    {
        return exclusiveUpper > 0 ? static_cast<int>(next() % static_cast<quint32>(exclusiveUpper)) : 0;
    }

private:
    quint32 m_state;
};

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

bool validateResponse(const K500Response &response, QString *error)
{
    const auto fail = [error](const QString &message) {
        if (error)
            *error = message;
        return false;
    };

    if (response.raw.size() < 5)
        return fail(QStringLiteral("parser emitted frame shorter than 5 bytes"));
    if (u8(response.raw.at(0)) != 0x55)
        return fail(QStringLiteral("parser emitted frame without 0x55 response header"));

    const int bodyLength = u8(response.raw.at(1)) | (int(u8(response.raw.at(2))) << 8);
    if (bodyLength <= 0 || bodyLength > 4096)
        return fail(QStringLiteral("parser emitted invalid body length %1").arg(bodyLength));

    const int expectedTotal = 1 + 2 + bodyLength + 1;
    if (response.raw.size() != expectedTotal)
        return fail(QStringLiteral("parser emitted size %1 for declared body %2")
                        .arg(response.raw.size()).arg(bodyLength));
    if (response.rsp != u8(response.raw.at(3)))
        return fail(QStringLiteral("response opcode does not match raw frame"));
    if (response.data.size() != bodyLength - 1)
        return fail(QStringLiteral("response data size does not match declared body"));
    if (response.data != response.raw.mid(4, bodyLength - 1))
        return fail(QStringLiteral("response data differs from raw payload"));
    if (response.checksumOk != K500Frame::verify(response.raw))
        return fail(QStringLiteral("checksum flag differs from verifier"));
    return true;
}

bool validateAll(const QList<K500Response> &responses, QString *error)
{
    for (const K500Response &response : responses) {
        if (!validateResponse(response, error))
            return false;
    }
    return true;
}

QByteArray randomBytes(XorShift32 &rng, int size)
{
    QByteArray bytes(size, Qt::Uninitialized);
    for (int i = 0; i < size; ++i)
        bytes[i] = char(rng.next() & 0xFFu);
    return bytes;
}

bool exhaustiveValidSplits(QString *error)
{
    QByteArray payload(0x40, Qt::Uninitialized);
    for (int i = 0; i < payload.size(); ++i)
        payload[i] = char((i * 37 + 11) & 0xFF);

    const QByteArray frame = responseFrame(0xBF, payload);
    for (int split = 0; split <= frame.size(); ++split) {
        K500ResponseParser parser;
        const auto first = parser.feed(frame.left(split));
        if (split < frame.size() && !first.isEmpty()) {
            if (error)
                *error = QStringLiteral("valid frame emitted before split continuation at %1").arg(split);
            return false;
        }

        auto responses = parser.feed(frame.mid(split));
        if (split == frame.size()) {
            responses = first;
        }
        if (responses.size() != 1 || responses.front().rsp != 0xBF
            || !responses.front().checksumOk || responses.front().data != payload) {
            if (error)
                *error = QStringLiteral("valid split recovery failed at boundary %1").arg(split);
            return false;
        }
        if (!validateAll(responses, error))
            return false;
    }
    return true;
}

bool malformedLengthRecovery(QString *error)
{
    const QByteArray sentinel = responseFrame(0xE3, QByteArray(1, char(0x01)));
    const QList<QByteArray> prefixes = {
        QByteArray::fromHex("550000"),       // body = 0
        QByteArray::fromHex("550110"),       // body = 4097
        QByteArray::fromHex("55FFFF"),       // body = 65535
        QByteArray::fromHex("0011223344550000"),
        QByteArray(128, char(0xAA)),
    };

    for (const QByteArray &prefix : prefixes) {
        K500ResponseParser parser;
        const auto responses = parser.feed(prefix + sentinel);
        if (!validateAll(responses, error))
            return false;
        if (responses.isEmpty() || responses.back().rsp != 0xE3
            || !responses.back().checksumOk) {
            if (error)
                *error = QStringLiteral("parser did not recover after malformed length/noise prefix");
            return false;
        }
    }
    return true;
}

bool deterministicFuzz(QString *error)
{
    K500ResponseParser parser;
    XorShift32 rng(InitialSeed);
    int emitted = 0;
    int validSentinels = 0;

    for (int iteration = 0; iteration < FuzzIterations; ++iteration) {
        // Bound retained incomplete-state lifetime during random fuzz. Separate
        // tests above verify intentional split-frame persistence exhaustively.
        if ((iteration & 0xFF) == 0)
            parser.reset();

        QByteArray chunk;
        switch (iteration % 11) {
        case 0:
            chunk = randomBytes(rng, rng.bounded(129));
            break;
        case 1:
            chunk = QByteArray::fromHex("550000") + randomBytes(rng, rng.bounded(32));
            break;
        case 2:
            chunk = QByteArray::fromHex("550110") + randomBytes(rng, rng.bounded(32));
            break;
        case 3:
            chunk = QByteArray::fromHex("55FFFF") + randomBytes(rng, rng.bounded(32));
            break;
        case 4: {
            QByteArray corrupt = responseFrame(quint8(rng.next() & 0xFFu),
                                               randomBytes(rng, rng.bounded(96)));
            corrupt[corrupt.size() - 1] = char(u8(corrupt.back()) ^ 0x5Au);
            chunk = std::move(corrupt);
            break;
        }
        case 5: {
            // Truncated, otherwise well-formed response.
            const QByteArray frame = responseFrame(0xBF, randomBytes(rng, rng.bounded(128)));
            chunk = frame.left(rng.bounded(frame.size()));
            break;
        }
        case 6:
            chunk = QByteArray(rng.bounded(96), char(0x55));
            break;
        case 7:
            chunk = QByteArray(rng.bounded(96), char(0x00));
            break;
        case 8:
            chunk = randomBytes(rng, rng.bounded(16))
                  + responseFrame(0xC0, randomBytes(rng, rng.bounded(24)));
            break;
        case 9:
            chunk = responseFrame(0xE3, QByteArray(1, char(0x01)));
            break;
        default:
            chunk = randomBytes(rng, rng.bounded(256));
            break;
        }

        const auto responses = parser.feed(chunk);
        if (!validateAll(responses, error)) {
            if (error)
                *error = QStringLiteral("iteration %1: %2").arg(iteration).arg(*error);
            return false;
        }
        emitted += responses.size();

        // Periodically prove that a reset parser still accepts a known-good
        // response after arbitrary preceding input.
        if ((iteration % 997) == 996) {
            parser.reset();
            const QByteArray sentinel = responseFrame(0xE3, QByteArray(1, char(0x01)));
            const int split = rng.bounded(sentinel.size() + 1);
            const auto first = parser.feed(sentinel.left(split));
            const auto second = parser.feed(sentinel.mid(split));
            QList<K500Response> combined = first;
            combined.append(second);
            if (combined.size() != 1 || combined.front().rsp != 0xE3
                || !combined.front().checksumOk) {
                if (error)
                    *error = QStringLiteral("sentinel recovery failed at fuzz iteration %1").arg(iteration);
                return false;
            }
            if (!validateAll(combined, error))
                return false;
            ++validSentinels;
        }
    }

    if (validSentinels < 10) {
        if (error)
            *error = QStringLiteral("fuzz harness executed too few valid sentinel checks");
        return false;
    }

    qInfo().noquote()
        << QStringLiteral("P4 parser fuzz completed: iterations=%1 emitted=%2 sentinels=%3 seed=0x%4")
               .arg(FuzzIterations).arg(emitted).arg(validSentinels)
               .arg(InitialSeed, 8, 16, QLatin1Char('0'));
    return true;
}
}

int main()
{
    // P4_PARSER_FUZZ_V1
    QString error;
    if (!K500ResponseParser::selfTest(&error)) {
        qCritical().noquote() << "Baseline parser self-test failed:" << error;
        return 1;
    }
    if (!exhaustiveValidSplits(&error)) {
        qCritical().noquote() << "P4 split-boundary test failed:" << error;
        return 2;
    }
    if (!malformedLengthRecovery(&error)) {
        qCritical().noquote() << "P4 malformed-length recovery failed:" << error;
        return 3;
    }
    if (!deterministicFuzz(&error)) {
        qCritical().noquote() << "P4 deterministic fuzz failed:" << error;
        return 4;
    }

    qInfo() << "P4 parser fuzz self-test passed";
    return 0;
}
