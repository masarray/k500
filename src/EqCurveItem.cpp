#include "EqCurveItem.h"

#include <QAbstractItemModel>
#include <QSGGeometryNode>
#include <QSGVertexColorMaterial>
#include <QtMath>

#include <algorithm>
#include <cmath>

namespace {
constexpr double Pi = 3.14159265358979323846;
constexpr double SampleRate = 48000.0;

class CurveSceneNode final : public QSGNode
{
public:
    explicit CurveSceneNode(int bandCount, int sampleCount)
        : bandCount(bandCount)
    {
        fill = createFillNode(sampleCount);
        appendChildNode(fill);

        bands.reserve(bandCount);
        for (int i = 0; i < bandCount; ++i) {
            auto *node = createStrokeNode(sampleCount);
            bands.push_back(node);
            appendChildNode(node);
        }

        crossover = createStrokeNode(sampleCount);
        totalShadow = createStrokeNode(sampleCount);
        totalGlow = createStrokeNode(sampleCount);
        total = createStrokeNode(sampleCount);
        appendChildNode(crossover);
        appendChildNode(totalShadow);
        appendChildNode(totalGlow);
        appendChildNode(total);
    }

    static QSGGeometryNode *createNode(QSGGeometry *geometry)
    {
        auto *node = new QSGGeometryNode;
        auto *material = new QSGVertexColorMaterial;
        material->setFlag(QSGMaterial::Blending, true);
        node->setGeometry(geometry);
        node->setMaterial(material);
        node->setFlag(QSGNode::OwnsGeometry, true);
        node->setFlag(QSGNode::OwnsMaterial, true);
        return node;
    }

    static QSGGeometryNode *createFillNode(int sampleCount)
    {
        auto *geometry = new QSGGeometry(QSGGeometry::defaultAttributes_ColoredPoint2D(), sampleCount * 2);
        geometry->setDrawingMode(QSGGeometry::DrawTriangleStrip);
        geometry->setVertexDataPattern(QSGGeometry::DynamicPattern);
        return createNode(geometry);
    }

    // P1_NATIVE_PEQ_ANALYTIC_AA_V1
    // Each stroke owns four vertices per response sample: transparent outer,
    // opaque inner, opaque inner, transparent outer. Three indexed quads per
    // segment form a feathered edge/core/edge strip. This is the same basic
    // vertex-AA principle Qt uses for clean primitive edges, but scoped only to
    // the PEQ geometry: no full-window MSAA, no Canvas and no offscreen layer.
    static QSGGeometryNode *createStrokeNode(int sampleCount)
    {
        constexpr int VerticesPerSample = 4;
        constexpr int IndicesPerSegment = 18;
        auto *geometry = new QSGGeometry(QSGGeometry::defaultAttributes_ColoredPoint2D(),
                                         sampleCount * VerticesPerSample,
                                         std::max(0, sampleCount - 1) * IndicesPerSegment,
                                         QSGGeometry::UnsignedShortType);
        geometry->setDrawingMode(QSGGeometry::DrawTriangles);
        geometry->setVertexDataPattern(QSGGeometry::DynamicPattern);
        geometry->setIndexDataPattern(QSGGeometry::StaticPattern);

        auto *indices = geometry->indexDataAsUShort();
        for (int i = 0; i < sampleCount - 1; ++i) {
            const quint16 a = static_cast<quint16>(i * VerticesPerSample);
            const quint16 b = static_cast<quint16>((i + 1) * VerticesPerSample);
            const quint16 pattern[IndicesPerSegment] = {
                static_cast<quint16>(a + 0), static_cast<quint16>(b + 0), static_cast<quint16>(a + 1),
                static_cast<quint16>(a + 1), static_cast<quint16>(b + 0), static_cast<quint16>(b + 1),
                static_cast<quint16>(a + 1), static_cast<quint16>(b + 1), static_cast<quint16>(a + 2),
                static_cast<quint16>(a + 2), static_cast<quint16>(b + 1), static_cast<quint16>(b + 2),
                static_cast<quint16>(a + 2), static_cast<quint16>(b + 2), static_cast<quint16>(a + 3),
                static_cast<quint16>(a + 3), static_cast<quint16>(b + 2), static_cast<quint16>(b + 3)
            };
            std::copy(std::begin(pattern), std::end(pattern), indices);
            indices += IndicesPerSegment;
        }
        return createNode(geometry);
    }

    int bandCount = 0;
    QSGGeometryNode *fill = nullptr;
    QVector<QSGGeometryNode *> bands;
    QSGGeometryNode *crossover = nullptr;
    QSGGeometryNode *totalShadow = nullptr;
    QSGGeometryNode *totalGlow = nullptr;
    QSGGeometryNode *total = nullptr;
};

QColor withAlpha(const QColor &color, int alpha)
{
    QColor result = color;
    result.setAlpha(std::clamp(alpha, 0, 255));
    return result;
}

QColor interpolate(const QColor &a, const QColor &b, double t)
{
    t = std::clamp(t, 0.0, 1.0);
    return QColor::fromRgbF(a.redF() + (b.redF() - a.redF()) * t,
                            a.greenF() + (b.greenF() - a.greenF()) * t,
                            a.blueF() + (b.blueF() - a.blueF()) * t,
                            a.alphaF() + (b.alphaF() - a.alphaF()) * t);
}

constexpr uchar premultipliedChannel(int channel, int alpha)
{
    return static_cast<uchar>((channel * alpha + 127) / 255);
}

// P1_NATIVE_PEQ_PREMULTIPLIED_ALPHA_V2
// QSGVertexColorMaterial blends premultiplied vertex colors. Supplying straight
// RGB with a low alpha makes translucent cyan/amber geometry appear almost
// opaque. Keep the retained native renderer, but feed it correct premultiplied
// colors so the graph has the restrained transparency of a professional EQ UI.
void setVertex(QSGGeometry::ColoredPoint2D &vertex, qreal x, qreal y, const QColor &color)
{
    const int alpha = color.alpha();
    vertex.set(static_cast<float>(x), static_cast<float>(y),
               premultipliedChannel(color.red(), alpha),
               premultipliedChannel(color.green(), alpha),
               premultipliedChannel(color.blue(), alpha),
               static_cast<uchar>(alpha));
}

static_assert(premultipliedChannel(242, 34) == 32,
              "PEQ scene-graph vertex colors must remain premultiplied");
}

EqCurveItem::EqCurveItem(QQuickItem *parent)
    : QQuickItem(parent)
{
    setFlag(ItemHasContents, true);
    initializeFrequencyGrid();
    m_crossoverResponse.fill(0.0f, SampleCount);
    m_totalResponse.fill(0.0f, SampleCount);
}

void EqCurveItem::disconnectModel()
{
    for (const auto &connection : std::as_const(m_modelConnections))
        QObject::disconnect(connection);
    m_modelConnections.clear();
}

void EqCurveItem::connectModel()
{
    if (!m_bandModel)
        return;

    m_modelConnections.push_back(connect(m_bandModel, &QAbstractItemModel::dataChanged, this,
        [this](const QModelIndex &topLeft, const QModelIndex &bottomRight, const QList<int> &) {
            if (!m_bandModel || m_bandResponses.size() != m_bandModel->rowCount()) {
                rebuildAllResponses();
            } else {
                const int first = std::max(0, topLeft.row());
                const int last = std::min(m_bandModel->rowCount() - 1, bottomRight.row());
                for (int row = first; row <= last; ++row)
                    rebuildBandResponse(row);
            }
            update();
        }));

    m_modelConnections.push_back(connect(m_bandModel, &QAbstractItemModel::modelReset, this, [this] {
        rebuildAllResponses();
        update();
    }));
    m_modelConnections.push_back(connect(m_bandModel, &QAbstractItemModel::rowsInserted, this,
        [this](const QModelIndex &, int, int) {
            rebuildAllResponses();
            update();
        }));
    m_modelConnections.push_back(connect(m_bandModel, &QAbstractItemModel::rowsRemoved, this,
        [this](const QModelIndex &, int, int) {
            rebuildAllResponses();
            update();
        }));
    m_modelConnections.push_back(connect(m_bandModel, &EqBandModel::crossoverChanged, this, [this] {
        rebuildCrossoverResponse();
        update();
    }));
}

void EqCurveItem::setBandModel(EqBandModel *model)
{
    if (m_bandModel == model)
        return;
    disconnectModel();
    m_bandModel = model;
    connectModel();
    rebuildAllResponses();
    emit bandModelChanged();
    update();
}

void EqCurveItem::setSelectedIndex(int value)
{
    if (m_selectedIndex == value)
        return;
    m_selectedIndex = value;
    emit selectedIndexChanged();
    update();
}

void EqCurveItem::setSelectedTarget(const QString &value)
{
    if (m_selectedTarget == value)
        return;
    m_selectedTarget = value;
    emit selectedTargetChanged();
    update();
}

void EqCurveItem::setLeftPad(qreal value)
{
    if (qFuzzyCompare(m_leftPad, value))
        return;
    m_leftPad = value;
    emit geometryParametersChanged();
    update();
}

void EqCurveItem::setRightPad(qreal value)
{
    if (qFuzzyCompare(m_rightPad, value))
        return;
    m_rightPad = value;
    emit geometryParametersChanged();
    update();
}

void EqCurveItem::setTopPad(qreal value)
{
    if (qFuzzyCompare(m_topPad, value))
        return;
    m_topPad = value;
    emit geometryParametersChanged();
    update();
}

void EqCurveItem::setPlotBottom(qreal value)
{
    if (qFuzzyCompare(m_plotBottom, value))
        return;
    m_plotBottom = value;
    emit geometryParametersChanged();
    update();
}

void EqCurveItem::setAccentColor(const QColor &value)
{
    if (m_accentColor == value)
        return;
    m_accentColor = value;
    emit colorsChanged();
    update();
}

void EqCurveItem::setAmberColor(const QColor &value)
{
    if (m_amberColor == value)
        return;
    m_amberColor = value;
    emit colorsChanged();
    update();
}

void EqCurveItem::initializeFrequencyGrid()
{
    m_frequencies.resize(SampleCount);
    m_cos1.resize(SampleCount);
    m_sin1.resize(SampleCount);
    m_cos2.resize(SampleCount);
    m_sin2.resize(SampleCount);

    for (int i = 0; i < SampleCount; ++i) {
        const double t = static_cast<double>(i) / static_cast<double>(SampleCount - 1);
        const double frequency = 20.0 * std::pow(1000.0, t);
        const double w = 2.0 * Pi * std::clamp(frequency, 1.0, 23999.0) / SampleRate;
        m_frequencies[i] = frequency;
        m_cos1[i] = std::cos(w);
        m_sin1[i] = std::sin(w);
        m_cos2[i] = std::cos(2.0 * w);
        m_sin2[i] = std::sin(2.0 * w);
    }
}

EqCurveItem::Biquad EqCurveItem::coefficientsForBand(int index) const
{
    Biquad coeff;
    if (!m_bandModel || index < 0 || index >= m_bandModel->rowCount())
        return coeff;

    const QModelIndex modelIndex = m_bandModel->index(index, 0);
    const double frequency = std::clamp(m_bandModel->data(modelIndex, EqBandModel::FrequencyRole).toDouble(), 20.0, 20000.0);
    const double gain = std::clamp(m_bandModel->data(modelIndex, EqBandModel::GainRole).toDouble(), -24.0, 24.0);
    const double q = std::clamp(m_bandModel->data(modelIndex, EqBandModel::QRole).toDouble(), 0.1, 30.0);
    const QString type = m_bandModel->data(modelIndex, EqBandModel::TypeNameRole).toString().trimmed().toUpper();

    if (std::abs(gain) < 0.001)
        return coeff;

    const double A = std::pow(10.0, gain / 40.0);
    const double w = 2.0 * Pi * std::clamp(frequency, 1.0, SampleRate / 2.0 - 1.0) / SampleRate;
    const double c = std::cos(w);
    const double s = std::sin(w);

    if (type.contains(QStringLiteral("LOW")) || type == QStringLiteral("LS")
        || type.contains(QStringLiteral("HIGH")) || type == QStringLiteral("HS")) {
        const bool high = type.contains(QStringLiteral("HIGH")) || type == QStringLiteral("HS");
        const double slope = std::clamp(q, 0.1, 10.0);
        const double radicand = std::max(0.000001, (A + 1.0 / A) * (1.0 / slope - 1.0) + 2.0);
        const double alpha = s * 0.5 * std::sqrt(radicand);
        const double beta = 2.0 * std::sqrt(A) * alpha;
        double a0 = 1.0;

        if (!high) {
            a0 = (A + 1.0) + (A - 1.0) * c + beta;
            coeff.b0 = A * ((A + 1.0) - (A - 1.0) * c + beta) / a0;
            coeff.b1 = 2.0 * A * ((A - 1.0) - (A + 1.0) * c) / a0;
            coeff.b2 = A * ((A + 1.0) - (A - 1.0) * c - beta) / a0;
            coeff.a1 = -2.0 * ((A - 1.0) + (A + 1.0) * c) / a0;
            coeff.a2 = ((A + 1.0) + (A - 1.0) * c - beta) / a0;
        } else {
            a0 = (A + 1.0) - (A - 1.0) * c + beta;
            coeff.b0 = A * ((A + 1.0) + (A - 1.0) * c + beta) / a0;
            coeff.b1 = -2.0 * A * ((A - 1.0) + (A + 1.0) * c) / a0;
            coeff.b2 = A * ((A + 1.0) + (A - 1.0) * c - beta) / a0;
            coeff.a1 = 2.0 * ((A - 1.0) - (A + 1.0) * c) / a0;
            coeff.a2 = ((A + 1.0) - (A - 1.0) * c - beta) / a0;
        }
    } else {
        const double alpha = s / (2.0 * q);
        const double a0 = 1.0 + alpha / A;
        coeff.b0 = (1.0 + alpha * A) / a0;
        coeff.b1 = -2.0 * c / a0;
        coeff.b2 = (1.0 - alpha * A) / a0;
        coeff.a1 = -2.0 * c / a0;
        coeff.a2 = (1.0 - alpha / A) / a0;
    }

    coeff.active = true;
    return coeff;
}

double EqCurveItem::magnitudeDb(const Biquad &coeff, double c1, double s1, double c2, double s2)
{
    if (!coeff.active)
        return 0.0;
    const double br = coeff.b0 + coeff.b1 * c1 + coeff.b2 * c2;
    const double bi = -(coeff.b1 * s1 + coeff.b2 * s2);
    const double ar = 1.0 + coeff.a1 * c1 + coeff.a2 * c2;
    const double ai = -(coeff.a1 * s1 + coeff.a2 * s2);
    const double numerator = br * br + bi * bi;
    const double denominator = std::max(1e-12, ar * ar + ai * ai);
    return 10.0 * std::log(std::max(numerator / denominator, 1e-12)) / std::log(10.0);
}

QVector<float> EqCurveItem::calculateBandResponse(int index) const
{
    QVector<float> response(SampleCount, 0.0f);
    const Biquad coeff = coefficientsForBand(index);
    if (!coeff.active)
        return response;

    for (int i = 0; i < SampleCount; ++i)
        response[i] = static_cast<float>(magnitudeDb(coeff, m_cos1[i], m_sin1[i], m_cos2[i], m_sin2[i]));
    return response;
}

double EqCurveItem::besselMagnitude(int order, double ratio)
{
    static constexpr double c2[] = {3.0, 3.0, 1.0};
    static constexpr double c3[] = {15.0, 15.0, 6.0, 1.0};
    static constexpr double c4[] = {105.0, 105.0, 45.0, 10.0, 1.0};
    const double *coefficients = order == 4 ? c4 : order == 3 ? c3 : c2;
    const int count = order == 4 ? 5 : order == 3 ? 4 : 3;
    const double scale = order == 4 ? 2.113917674904216 : order == 3 ? 1.7556723686812106 : 1.3616541287161308;
    const double x = std::max(0.0, ratio) * scale;
    double real = 0.0;
    double imag = 0.0;
    for (int power = 0; power < count; ++power) {
        const double magnitude = coefficients[power] * std::pow(x, power);
        const double phase = power * Pi / 2.0;
        real += magnitude * std::cos(phase);
        imag += magnitude * std::sin(phase);
    }
    return coefficients[0] / std::max(1e-12, std::sqrt(real * real + imag * imag));
}

double EqCurveItem::crossoverOneDb(bool lowPass, const QString &label, double cutoff, double frequency)
{
    const QString upper = label.trimmed().toUpper();
    const int order = upper.contains(QStringLiteral("24")) ? 4 : upper.contains(QStringLiteral("18")) ? 3 : 2;
    const double ratio = lowPass ? std::max(frequency, 1.0) / std::max(cutoff, 1.0)
                                 : std::max(cutoff, 1.0) / std::max(frequency, 1.0);
    double magnitude = 1.0;
    if (upper.contains(QStringLiteral("BESSEL"))) {
        magnitude = besselMagnitude(order, ratio);
    } else if (upper.contains(QStringLiteral("LR"))) {
        const double butter = 1.0 / std::sqrt(1.0 + std::pow(ratio, 4.0));
        magnitude = butter * butter;
    } else {
        magnitude = 1.0 / std::sqrt(1.0 + std::pow(ratio, 2.0 * order));
    }
    return 20.0 * std::log(std::max(magnitude, 1e-12)) / std::log(10.0);
}

float EqCurveItem::calculateCrossoverDb(double frequency) const
{
    if (!m_bandModel)
        return 0.0f;
    double db = 0.0;
    const double hpf = m_bandModel->hpfHz();
    const double lpf = m_bandModel->lpfHz();
    if (hpf > 20.001)
        db += crossoverOneDb(false, m_bandModel->hpType(), hpf, frequency);
    if (lpf < 19999.999)
        db += crossoverOneDb(true, m_bandModel->lpType(), lpf, frequency);
    return static_cast<float>(db);
}

void EqCurveItem::rebuildAllResponses()
{
    const int count = m_bandModel ? m_bandModel->rowCount() : 0;
    m_bandResponses.clear();
    m_bandResponses.resize(count);
    m_crossoverResponse.fill(0.0f, SampleCount);
    m_totalResponse.fill(0.0f, SampleCount);

    if (!m_bandModel)
        return;

    for (int sample = 0; sample < SampleCount; ++sample) {
        m_crossoverResponse[sample] = calculateCrossoverDb(m_frequencies[sample]);
        m_totalResponse[sample] = m_crossoverResponse[sample];
    }

    for (int band = 0; band < count; ++band) {
        m_bandResponses[band] = calculateBandResponse(band);
        for (int sample = 0; sample < SampleCount; ++sample)
            m_totalResponse[sample] += m_bandResponses[band][sample];
    }
}

void EqCurveItem::rebuildBandResponse(int index)
{
    if (!m_bandModel || index < 0 || index >= m_bandModel->rowCount()
        || m_bandResponses.size() != m_bandModel->rowCount()
        || m_totalResponse.size() != SampleCount) {
        rebuildAllResponses();
        return;
    }

    const QVector<float> next = calculateBandResponse(index);
    QVector<float> &previous = m_bandResponses[index];
    if (previous.size() != SampleCount) {
        rebuildAllResponses();
        return;
    }
    for (int sample = 0; sample < SampleCount; ++sample)
        m_totalResponse[sample] += next[sample] - previous[sample];
    previous = next;
}

void EqCurveItem::rebuildCrossoverResponse()
{
    if (!m_bandModel || m_totalResponse.size() != SampleCount) {
        rebuildAllResponses();
        return;
    }

    for (int sample = 0; sample < SampleCount; ++sample) {
        float bandSum = 0.0f;
        for (const auto &response : std::as_const(m_bandResponses)) {
            if (response.size() == SampleCount)
                bandSum += response[sample];
        }
        m_crossoverResponse[sample] = calculateCrossoverDb(m_frequencies[sample]);
        m_totalResponse[sample] = m_crossoverResponse[sample] + bandSum;
    }
}

qreal EqCurveItem::yForDb(double db) const
{
    const double visibleDb = std::clamp(db, -24.0, 24.0);
    const qreal height = std::max<qreal>(1.0, m_plotBottom - m_topPad);
    return m_topPad + (24.0 - visibleDb) / 48.0 * height;
}

QSGNode *EqCurveItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    const int bandCount = m_bandResponses.size();
    if (width() <= 1.0 || height() <= 1.0 || m_totalResponse.size() != SampleCount) {
        delete oldNode;
        return nullptr;
    }

    auto *root = static_cast<CurveSceneNode *>(oldNode);
    if (!root || root->bandCount != bandCount) {
        delete root;
        root = new CurveSceneNode(bandCount, SampleCount);
    }

    const qreal plotWidth = std::max<qreal>(1.0, width() - m_leftPad - m_rightPad);
    const qreal zeroY = yForDb(0.0);

    auto xForSample = [this, plotWidth](int sample) -> qreal {
        return m_leftPad + static_cast<qreal>(sample) / static_cast<qreal>(SampleCount - 1) * plotWidth;
    };

    auto writeStroke = [&](QSGGeometryNode *node, const QVector<float> &response,
                           qreal thickness, qreal feather, const auto &colorAt) {
        auto *geometry = node->geometry();
        auto *vertices = geometry->vertexDataAsColoredPoint2D();
        const qreal innerHalf = std::max<qreal>(0.05, thickness * 0.5);
        const qreal outerHalf = innerHalf + std::max<qreal>(0.4, feather);
        for (int i = 0; i < SampleCount; ++i) {
            const int previous = std::max(0, i - 1);
            const int next = std::min(SampleCount - 1, i + 1);
            const qreal x = xForSample(i);
            const qreal y = yForDb(response.value(i));
            const qreal dx = xForSample(next) - xForSample(previous);
            const qreal dy = yForDb(response.value(next)) - yForDb(response.value(previous));
            const qreal length = std::max<qreal>(0.0001, std::hypot(dx, dy));
            const qreal nx = -dy / length;
            const qreal ny = dx / length;
            const QColor color = colorAt(i);
            QColor edgeColor = color;
            edgeColor.setAlpha(0);

            setVertex(vertices[i * 4 + 0], x + nx * outerHalf, y + ny * outerHalf, edgeColor);
            setVertex(vertices[i * 4 + 1], x + nx * innerHalf, y + ny * innerHalf, color);
            setVertex(vertices[i * 4 + 2], x - nx * innerHalf, y - ny * innerHalf, color);
            setVertex(vertices[i * 4 + 3], x - nx * outerHalf, y - ny * outerHalf, edgeColor);
        }
        node->markDirty(QSGNode::DirtyGeometry);
    };

    // Professional EQ hierarchy: one clean composite response carries the eye;
    // fill, individual filters, crossover and glow only support it. The feather
    // geometry keeps all strokes smooth without a full-window MSAA tax.
    {
        auto *geometry = root->fill->geometry();
        auto *vertices = geometry->vertexDataAsColoredPoint2D();
        const QColor topColor = withAlpha(m_accentColor, 20);
        const QColor bottomColor = withAlpha(m_accentColor, 0);
        for (int i = 0; i < SampleCount; ++i) {
            const qreal x = xForSample(i);
            setVertex(vertices[i * 2], x, yForDb(m_totalResponse[i]), topColor);
            setVertex(vertices[i * 2 + 1], x, zeroY, bottomColor);
        }
        root->fill->markDirty(QSGNode::DirtyGeometry);
    }

    for (int band = 0; band < bandCount; ++band) {
        const bool active = std::any_of(m_bandResponses[band].cbegin(), m_bandResponses[band].cend(),
                                        [](float value) { return std::abs(value) > 0.001f; });
        const bool selected = active && m_selectedTarget == QStringLiteral("band") && band == m_selectedIndex;
        const QColor color = active ? withAlpha(m_amberColor, selected ? 150 : 22)
                                    : QColor(0, 0, 0, 0);
        writeStroke(root->bands[band], m_bandResponses[band], selected ? 1.35 : 0.75,
                    selected ? 0.80 : 0.65, [color](int) { return color; });
    }

    writeStroke(root->crossover, m_crossoverResponse, 0.90, 0.70,
                [this](int) { return withAlpha(m_amberColor, 68); });
    writeStroke(root->totalShadow, m_totalResponse, 3.6, 1.00,
                [](int) { return QColor(1, 2, 3, 164); });
    writeStroke(root->totalGlow, m_totalResponse, 4.6, 1.25,
                [this](int) { return withAlpha(m_accentColor, 22); });

    const QColor compositeColor = interpolate(m_accentColor, QColor(220, 253, 255), 0.16);
    writeStroke(root->total, m_totalResponse, 2.15, 0.82,
                [compositeColor](int) { return compositeColor; });

    return root;
}

void EqCurveItem::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    if (newGeometry.size() != oldGeometry.size())
        update();
}
