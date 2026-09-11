#pragma once

#include <QColor>
#include <QMetaObject>
#include <QQuickItem>
#include <QString>
#include <QVector>
#include <QtQml/qqmlregistration.h>

#include "StudioEngine.h"

class EqCurveItem : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(EqBandModel *bandModel READ bandModel WRITE setBandModel NOTIFY bandModelChanged)
    Q_PROPERTY(int selectedIndex READ selectedIndex WRITE setSelectedIndex NOTIFY selectedIndexChanged)
    Q_PROPERTY(QString selectedTarget READ selectedTarget WRITE setSelectedTarget NOTIFY selectedTargetChanged)
    Q_PROPERTY(qreal leftPad READ leftPad WRITE setLeftPad NOTIFY geometryParametersChanged)
    Q_PROPERTY(qreal rightPad READ rightPad WRITE setRightPad NOTIFY geometryParametersChanged)
    Q_PROPERTY(qreal topPad READ topPad WRITE setTopPad NOTIFY geometryParametersChanged)
    Q_PROPERTY(qreal plotBottom READ plotBottom WRITE setPlotBottom NOTIFY geometryParametersChanged)
    Q_PROPERTY(QColor accentColor READ accentColor WRITE setAccentColor NOTIFY colorsChanged)
    Q_PROPERTY(QColor amberColor READ amberColor WRITE setAmberColor NOTIFY colorsChanged)

public:
    explicit EqCurveItem(QQuickItem *parent = nullptr);
    ~EqCurveItem() override = default;

    EqBandModel *bandModel() const { return m_bandModel; }
    int selectedIndex() const { return m_selectedIndex; }
    QString selectedTarget() const { return m_selectedTarget; }
    qreal leftPad() const { return m_leftPad; }
    qreal rightPad() const { return m_rightPad; }
    qreal topPad() const { return m_topPad; }
    qreal plotBottom() const { return m_plotBottom; }
    QColor accentColor() const { return m_accentColor; }
    QColor amberColor() const { return m_amberColor; }

    void setBandModel(EqBandModel *model);
    void setSelectedIndex(int value);
    void setSelectedTarget(const QString &value);
    void setLeftPad(qreal value);
    void setRightPad(qreal value);
    void setTopPad(qreal value);
    void setPlotBottom(qreal value);
    void setAccentColor(const QColor &value);
    void setAmberColor(const QColor &value);

    Q_INVOKABLE void requestPaint() { update(); }

signals:
    void bandModelChanged();
    void selectedIndexChanged();
    void selectedTargetChanged();
    void geometryParametersChanged();
    void colorsChanged();

protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) override;
    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

private:
    struct Biquad {
        double b0 = 1.0;
        double b1 = 0.0;
        double b2 = 0.0;
        double a1 = 0.0;
        double a2 = 0.0;
        bool active = false;
    };

    void disconnectModel();
    void connectModel();
    void initializeFrequencyGrid();
    void rebuildAllResponses();
    void rebuildBandResponse(int index);
    void rebuildCrossoverResponse();
    QVector<float> calculateBandResponse(int index) const;
    float calculateCrossoverDb(double frequency) const;
    Biquad coefficientsForBand(int index) const;
    static double magnitudeDb(const Biquad &coeff, double c1, double s1, double c2, double s2);
    static double besselMagnitude(int order, double ratio);
    static double crossoverOneDb(bool lowPass, const QString &label, double cutoff, double frequency);
    qreal yForDb(double db) const;

    EqBandModel *m_bandModel = nullptr;
    QVector<QMetaObject::Connection> m_modelConnections;

    // P1_NATIVE_PEQ_HIGH_RES_V1
    // 720 cached log-frequency samples keep curvature sub-pixel smooth at the
    // 1260-1484 px qualified window sizes while remaining tiny compared with a
    // CPU Canvas repaint. Trigonometry is still precomputed once per graph.
    static constexpr int SampleCount = 720;
    QVector<double> m_frequencies;
    QVector<double> m_cos1;
    QVector<double> m_sin1;
    QVector<double> m_cos2;
    QVector<double> m_sin2;
    QVector<QVector<float>> m_bandResponses;
    QVector<float> m_crossoverResponse;
    QVector<float> m_totalResponse;

    int m_selectedIndex = 0;
    QString m_selectedTarget = QStringLiteral("band");
    qreal m_leftPad = 56.0;
    qreal m_rightPad = 22.0;
    qreal m_topPad = 24.0;
    qreal m_plotBottom = 316.0;
    QColor m_accentColor = QColor(QStringLiteral("#24E9F2"));
    QColor m_amberColor = QColor(QStringLiteral("#F0B84B"));
};
