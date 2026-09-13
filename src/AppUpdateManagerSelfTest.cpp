#include "AppUpdateManager.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

bool AppUpdateManager::selfTest(QString *error)
{
    auto fail = [error](const QString &message) {
        if (error)
            *error = message;
        return false;
    };

    // UPDATE_METADATA_SELF_TEST_V1
    // Exercise the production manifest/checksum parsers without network access,
    // installer execution, or K500 hardware. This protects the exact trust
    // boundary used by in-app stable updates.
    AppUpdateManager manager;
    manager.m_latestVersion = QStringLiteral("9.9.9");
    manager.m_setupAssetName = QStringLiteral("SonKuPik-K500-v9.9.9-Windows-Setup.exe");
    manager.m_releaseSetupBytes = 2 * 1024 * 1024;

    const QByteArray goodHash(64, 'a');
    const qint64 goodBytes = manager.m_releaseSetupBytes;

    QJsonObject setupArtifact;
    setupArtifact.insert(QStringLiteral("file"), manager.m_setupAssetName);
    setupArtifact.insert(QStringLiteral("sha256"), QString::fromLatin1(goodHash));
    setupArtifact.insert(QStringLiteral("bytes"), double(goodBytes));

    QJsonArray artifacts;
    artifacts.append(setupArtifact);

    QJsonObject manifest;
    manifest.insert(QStringLiteral("schema"), QStringLiteral("sonkupik-k500-release-manifest-v3"));
    manifest.insert(QStringLiteral("product"), QStringLiteral("SonKuPik K500"));
    manifest.insert(QStringLiteral("channel"), QStringLiteral("stable"));
    manifest.insert(QStringLiteral("version"), manager.m_latestVersion);
    manifest.insert(QStringLiteral("target"), QStringLiteral("windows-x64"));
    manifest.insert(QStringLiteral("stableReleaseEligible"), true);
    manifest.insert(QStringLiteral("installerTechnology"), QStringLiteral("Inno Setup 6"));
    manifest.insert(QStringLiteral("artifacts"), artifacts);

    const QByteArray validManifest = QJsonDocument(manifest).toJson(QJsonDocument::Compact);
    if (!manager.validateManifest(validManifest))
        return fail(QStringLiteral("valid manifest rejected: %1").arg(manager.m_errorText));
    if (manager.m_manifestSha256 != goodHash || manager.m_manifestSetupBytes != goodBytes)
        return fail(QStringLiteral("valid manifest did not hydrate expected Setup identity"));

    const QByteArray validSums = goodHash + QByteArrayLiteral("  ")
        + manager.m_setupAssetName.toUtf8() + QByteArrayLiteral("\n");
    if (!manager.parseExpectedChecksum(validSums))
        return fail(QStringLiteral("valid SHA256SUMS entry rejected: %1").arg(manager.m_errorText));
    if (manager.m_expectedSha256 != goodHash)
        return fail(QStringLiteral("checksum parser did not preserve expected SHA-256"));

    // A second independently-published checksum that disagrees with the
    // release manifest must fail closed.
    manager.m_errorText.clear();
    manager.m_expectedSha256.clear();
    const QByteArray wrongHash(64, 'b');
    const QByteArray wrongSums = wrongHash + QByteArrayLiteral("  ")
        + manager.m_setupAssetName.toUtf8() + QByteArrayLiteral("\n");
    if (manager.parseExpectedChecksum(wrongSums))
        return fail(QStringLiteral("mismatched SHA256SUMS entry was accepted"));
    if (!manager.m_errorText.contains(QStringLiteral("disagrees"), Qt::CaseInsensitive))
        return fail(QStringLiteral("checksum mismatch did not report the fail-closed reason"));

    // Wrong platform metadata must never be accepted for the Windows updater.
    manager.m_errorText.clear();
    QJsonObject wrongTarget = manifest;
    wrongTarget.insert(QStringLiteral("target"), QStringLiteral("linux-x64"));
    if (manager.validateManifest(QJsonDocument(wrongTarget).toJson(QJsonDocument::Compact)))
        return fail(QStringLiteral("non-Windows stable manifest was accepted"));

    // Asset byte count is part of release identity and must agree with the
    // GitHub release metadata already discovered before download begins.
    manager.m_errorText.clear();
    QJsonObject wrongBytesArtifact = setupArtifact;
    wrongBytesArtifact.insert(QStringLiteral("bytes"), double(goodBytes + 1));
    QJsonArray wrongBytesArtifacts;
    wrongBytesArtifacts.append(wrongBytesArtifact);
    QJsonObject wrongBytesManifest = manifest;
    wrongBytesManifest.insert(QStringLiteral("artifacts"), wrongBytesArtifacts);
    if (manager.validateManifest(QJsonDocument(wrongBytesManifest).toJson(QJsonDocument::Compact)))
        return fail(QStringLiteral("manifest/GitHub asset size mismatch was accepted"));

    if (error)
        error->clear();
    return true;
}
