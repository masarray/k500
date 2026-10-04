#include "DonationPromptController.h"

#include <QFile>
#include <QSettings>
#include <QStringList>

namespace {
constexpr auto kObservedDaysKey = "support/donationPrompt/v1/observedDays";
constexpr auto kAcknowledgedKey = "support/donationPrompt/v1/acknowledged";
constexpr auto kAcknowledgedDateKey = "support/donationPrompt/v1/acknowledgedDate";
constexpr int kRequiredDistinctDays = 3;
constexpr auto kQrisResource = ":/support/qris-sonkupik.png";

QStringList normalizedObservedDays(const QVariant &stored)
{
    QStringList normalized;
    const QStringList raw = stored.toStringList();
    for (const QString &entry : raw) {
        const QDate date = QDate::fromString(entry, Qt::ISODate);
        if (!date.isValid())
            continue;

        const QString canonical = date.toString(Qt::ISODate);
        if (!normalized.contains(canonical))
            normalized.append(canonical);

        if (normalized.size() == kRequiredDistinctDays)
            break;
    }
    return normalized;
}
}

DonationPromptController::DonationPromptController(bool previewMode, QObject *parent)
    : QObject(parent),
      m_settings(std::make_unique<QSettings>()),
      m_previewMode(previewMode)
{
    initialize(QDate::currentDate());
}

DonationPromptController::~DonationPromptController() = default;

DonationPromptController::DonationPromptController(const QString &settingsFilePath,
                                                   const QDate &today,
                                                   bool previewMode,
                                                   QObject *parent)
    : QObject(parent),
      m_settings(std::make_unique<QSettings>(settingsFilePath, QSettings::IniFormat)),
      m_previewMode(previewMode)
{
    initialize(today);
}

bool DonationPromptController::qrisAvailable() const
{
    return QFile::exists(QString::fromLatin1(kQrisResource));
}

void DonationPromptController::initialize(const QDate &today)
{
    QStringList days =
        normalizedObservedDays(m_settings->value(QString::fromLatin1(kObservedDaysKey)));
    const bool acknowledged =
        m_settings->value(QString::fromLatin1(kAcknowledgedKey), false).toBool();

    // DONATION_PROMPT_DISTINCT_DAY_V1 — QA preview is deliberately read-only:
    // visual testing must not consume or accelerate a real user's day counter.
    if (!m_previewMode && !acknowledged && today.isValid()
        && days.size() < kRequiredDistinctDays) {
        const QString todayKey = today.toString(Qt::ISODate);
        if (!days.contains(todayKey)) {
            days.append(todayKey);
            m_settings->setValue(QString::fromLatin1(kObservedDaysKey), days);
            m_settings->sync();
        }
    }

    m_observedDayCount = qMin(days.size(), kRequiredDistinctDays);

    // DONATION_QRIS_FAIL_CLOSED_V1 — production never interrupts startup with
    // an unusable or fabricated payment surface. Preview mode is the explicit
    // QA escape hatch for visual review before the official PNG is supplied.
    m_shouldShow = m_previewMode
        || (!acknowledged
            && m_observedDayCount >= kRequiredDistinctDays
            && qrisAvailable());
}

void DonationPromptController::acknowledge()
{
    if (!m_shouldShow)
        return;

    if (!m_previewMode) {
        m_settings->setValue(QString::fromLatin1(kAcknowledgedKey), true);
        m_settings->setValue(QString::fromLatin1(kAcknowledgedDateKey),
                             QDate::currentDate().toString(Qt::ISODate));
        m_settings->sync();
    }

    m_shouldShow = false;
    emit shouldShowChanged();
}
