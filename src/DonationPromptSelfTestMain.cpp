#include "DonationPromptController.h"

#include <QCoreApplication>
#include <QDate>
#include <QDebug>
#include <QSettings>
#include <QTemporaryDir>

namespace {
bool expect(bool condition, const char *message)
{
    if (condition)
        return true;

    qCritical().noquote() << "Donation prompt self-test failed:" << message;
    return false;
}
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QTemporaryDir temp;
    if (!temp.isValid())
        return 2;

    const QString settingsPath =
        temp.filePath(QStringLiteral("donation-prompt.ini"));

    {
        DonationPromptController day1(settingsPath, QDate(2026, 10, 1));
        if (!expect(day1.observedDayCount() == 1, "day 1 must count once")
            || !expect(!day1.shouldShow(), "day 1 must not prompt"))
            return 3;
    }
    {
        DonationPromptController sameDay(settingsPath, QDate(2026, 10, 1));
        if (!expect(sameDay.observedDayCount() == 1,
                    "same-day reopen must not increment"))
            return 4;
    }
    {
        DonationPromptController preview(settingsPath, QDate(2026, 10, 2), true);
        if (!expect(preview.observedDayCount() == 1,
                    "preview must not advance the usage calendar")
            || !expect(preview.shouldShow(), "preview must force the UI open"))
            return 5;
        preview.acknowledge();
    }
    {
        QSettings settings(settingsPath, QSettings::IniFormat);
        if (!expect(!settings.value(
                        QStringLiteral("support/donationPrompt/v1/acknowledged"),
                        false).toBool(),
                    "preview acknowledgement must not persist"))
            return 6;
    }
    {
        DonationPromptController day2(settingsPath, QDate(2026, 10, 2));
        if (!expect(day2.observedDayCount() == 2,
                    "day 2 must be the second distinct day")
            || !expect(!day2.shouldShow(), "day 2 must not prompt"))
            return 7;
    }
    {
        DonationPromptController day3(settingsPath, QDate(2026, 10, 3));
        if (!expect(day3.observedDayCount() == 3,
                    "day 3 must reach eligibility"))
            return 8;
        // The self-test target intentionally carries no payment asset.
        if (!expect(!day3.qrisAvailable(),
                    "self-test must not contain a QRIS payment asset")
            || !expect(!day3.shouldShow(),
                       "production must fail closed without official QRIS"))
            return 9;
    }
    {
        DonationPromptController day4(settingsPath, QDate(2026, 10, 4));
        if (!expect(day4.observedDayCount() == 3,
                    "usage history must stay capped at three"))
            return 10;
    }

    qInfo() << "Donation prompt distinct-day self-test passed";
    return 0;
}
