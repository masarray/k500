#pragma once

#include <QDate>
#include <QObject>
#include <QString>

#include <memory>

class QSettings;

class DonationPromptController final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool shouldShow READ shouldShow NOTIFY shouldShowChanged)
    Q_PROPERTY(int observedDayCount READ observedDayCount CONSTANT)
    Q_PROPERTY(bool qrisAvailable READ qrisAvailable CONSTANT)
    Q_PROPERTY(bool previewMode READ previewMode CONSTANT)

public:
    explicit DonationPromptController(bool previewMode = false, QObject *parent = nullptr);
    DonationPromptController(const QString &settingsFilePath,
                             const QDate &today,
                             bool previewMode = false,
                             QObject *parent = nullptr);

    bool shouldShow() const { return m_shouldShow; }
    int observedDayCount() const { return m_observedDayCount; }
    bool qrisAvailable() const;
    bool previewMode() const { return m_previewMode; }

    Q_INVOKABLE void acknowledge();

signals:
    void shouldShowChanged();

private:
    void initialize(const QDate &today);

    std::unique_ptr<QSettings> m_settings;
    bool m_previewMode = false;
    bool m_shouldShow = false;
    int m_observedDayCount = 0;
};
