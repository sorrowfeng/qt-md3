#include "MdSteps.h"

#include "core/MdFocusRing.h"
#include "core/MdIcon.h"
#include "core/MdShape.h"
#include "core/MdStateLayer.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "styles/MdStyleBase.h"

#include <QtGui/QFontMetrics>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>

#include <cmath>

namespace md {

namespace {

/// Disc edge, in logical px. Matches the list leading-avatar edge the family
/// promotes (MdAvatar's 40 px default), so a step row lines up with a list.
constexpr qreal kDiscSize = 32.0;
/// Connector thickness. The progress-indicator linear track is 4 px; the
/// step connector is the quieter 2 px hairline the Divider family uses for
/// in-content rules.
constexpr qreal kConnectorThickness = 2.0;
/// Gap disc → label.
constexpr qreal kLabelGap = 8.0;
/// Gap disc → connector.
constexpr qreal kConnectorGap = 4.0;
/// Outer margin that leaves room for the outward focus ring.
constexpr qreal kFocusMargin = 8.0;

} // namespace

MdSteps::MdSteps(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdSteps::~MdSteps() = default;

void MdSteps::init()
{
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    setAccessibleName(QStringLiteral("steps"));
    setAttribute(Qt::WA_Hover, true);

    MdStyleBase::connectThemeUpdate(this, &MdSteps::onThemeChanged);
}

void MdSteps::setStepCount(int count)
{
    const int clamped = qMax(1, count);
    if (m_stepCount == clamped) {
        return;
    }
    m_stepCount = clamped;
    if (m_currentStep >= m_stepCount) {
        m_currentStep = m_stepCount - 1;
        emit currentStepChanged(m_currentStep);
    }
    updateGeometry();
    update();
    emit stepCountChanged(m_stepCount);
}

void MdSteps::setCurrentStep(int step)
{
    const int clamped = qBound(0, step, m_stepCount - 1);
    if (m_currentStep == clamped) {
        return;
    }
    m_currentStep = clamped;
    update();
    emit currentStepChanged(m_currentStep);
}

void MdSteps::setOrientation(MdStepsOrientation orientation)
{
    if (orientation >= MdStepsOrientation::Count) {
        return;
    }
    if (m_orientation == orientation) {
        return;
    }
    m_orientation = orientation;
    updateGeometry();
    update();
    emit orientationChanged(m_orientation);
}

void MdSteps::setLabels(const QStringList &labels)
{
    if (m_labels == labels) {
        return;
    }
    m_labels = labels;
    updateGeometry();
    update();
    emit labelsChanged(m_labels);
}

void MdSteps::setReadOnly(bool readOnly)
{
    if (m_readOnly == readOnly) {
        return;
    }
    m_readOnly = readOnly;
    emit readOnlyChanged(m_readOnly);
}

MdSteps::StepState MdSteps::stateAt(int index) const
{
    if (index < 0 || index >= m_stepCount) {
        return StepState::Upcoming;
    }
    if (index < m_currentStep) {
        return StepState::Complete;
    }
    if (index == m_currentStep) {
        return StepState::Current;
    }
    return StepState::Upcoming;
}

QColor MdSteps::resolvedDiscColor(StepState state) const
{
    const MdTheme &theme = MdTheme::instance();
    switch (state) {
    case StepState::Complete:
        // md.sys.color.primary — the filled progress reading.
        return theme.color(ColorRole::Primary);
    case StepState::Current:
        // md.sys.color.primary-container — the selected / current reading.
        return theme.color(ColorRole::PrimaryContainer);
    case StepState::Upcoming:
    case StepState::Count:
        break;
    }
    // md.sys.color.surface-container-highest — the placeholder reading.
    return theme.color(ColorRole::SurfaceContainerHighest);
}

QColor MdSteps::resolvedDiscContent(StepState state) const
{
    const MdTheme &theme = MdTheme::instance();
    switch (state) {
    case StepState::Complete:
        return theme.color(ColorRole::OnPrimary);
    case StepState::Current:
        return theme.color(ColorRole::OnPrimaryContainer);
    case StepState::Upcoming:
    case StepState::Count:
        break;
    }
    return theme.color(ColorRole::OnSurfaceVariant);
}

QColor MdSteps::resolvedConnectorColor(bool beforeComplete) const
{
    const MdTheme &theme = MdTheme::instance();
    // The progress-indicator pair: active-indicator.color vs track.color.
    return beforeComplete ? theme.color(ColorRole::Primary)
                          : theme.color(ColorRole::OutlineVariant);
}

QColor MdSteps::resolvedLabelColor(StepState state) const
{
    const MdTheme &theme = MdTheme::instance();
    if (state == StepState::Upcoming) {
        return theme.color(ColorRole::OnSurfaceVariant);
    }
    return theme.color(ColorRole::OnSurface);
}

QSize MdSteps::sizeHint() const
{
    const QFont font = MdTypeScale::font(TypeStyle::LabelMedium);
    const QFontMetrics metrics(font);
    int labelExtent = 0;
    for (const QString &label : m_labels) {
        labelExtent = qMax(labelExtent, metrics.horizontalAdvance(label));
    }

    const qreal disc = kDiscSize;
    const qreal span = m_stepCount * disc
                       + qMax(0, m_stepCount - 1) * (2.0 * kConnectorGap + 8.0);

    if (m_orientation == MdStepsOrientation::Horizontal) {
        const qreal height = kFocusMargin * 2.0 + disc + kLabelGap
                             + metrics.height();
        return QSize(qRound(span + kFocusMargin * 2.0
                            + labelExtent * 0.25),
                     qRound(height));
    }

    const qreal width = kFocusMargin * 2.0 + disc + kLabelGap + labelExtent;
    const qreal height = kFocusMargin * 2.0 + m_stepCount * disc
                         + qMax(0, m_stepCount - 1) * (2.0 * kConnectorGap + 8.0);
    return QSize(qRound(width), qRound(height));
}

QSize MdSteps::minimumSizeHint() const
{
    return QSize(qRound(kDiscSize + kFocusMargin * 2.0),
                 qRound(kDiscSize + kFocusMargin * 2.0));
}

qreal MdSteps::discRadius() const
{
    return kDiscSize / 2.0;
}

QVector<QPointF> MdSteps::discCentres() const
{
    QVector<QPointF> centres;
    for (int i = 0; i < m_stepCount; ++i) {
        if (m_orientation == MdStepsOrientation::Horizontal) {
            const qreal stepWidth = width() / qreal(m_stepCount);
            centres.append(QPointF(stepWidth * (i + 0.5), kFocusMargin + discRadius()));
        } else {
            const qreal stepHeight = height() / qreal(m_stepCount);
            centres.append(QPointF(kFocusMargin + discRadius(),
                                   stepHeight * (i + 0.5)));
        }
    }
    return centres;
}

int MdSteps::stepAtPosition(const QPointF &position) const
{
    const QVector<QPointF> centres = discCentres();
    const qreal radius = discRadius() + 4.0;
    for (int i = 0; i < centres.size(); ++i) {
        const QPointF delta = position - centres.at(i);
        if (delta.x() * delta.x() + delta.y() * delta.y()
            <= radius * radius) {
            return i;
        }
    }
    // Fall back to the nearest centre along the stack axis so the whole row
    // (including the label) is clickable, the way a wizard step usually is.
    int best = -1;
    qreal bestDistance = 0.0;
    for (int i = 0; i < centres.size(); ++i) {
        const QPointF delta = position - centres.at(i);
        const qreal distance = m_orientation == MdStepsOrientation::Horizontal
                                   ? std::fabs(delta.x())
                                   : std::fabs(delta.y());
        if (best < 0 || distance < bestDistance) {
            best = i;
            bestDistance = distance;
        }
    }
    return best;
}

void MdSteps::selectStep(int index)
{
    if (m_readOnly) {
        return;
    }
    const int clamped = qBound(0, index, m_stepCount - 1);
    setCurrentStep(clamped);
    emit stepSelected(clamped);
}

void MdSteps::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QFont labelFont = MdTypeScale::font(TypeStyle::LabelMedium);
    const QFontMetrics metrics(labelFont);
    const QVector<QPointF> centres = discCentres();
    const qreal radius = discRadius();

    // Connectors first, so the discs sit on top of their ends.
    for (int i = 0; i + 1 < centres.size(); ++i) {
        const bool beforeComplete = (i + 1) <= m_currentStep;
        const QColor colour = resolvedConnectorColor(beforeComplete);
        painter.setPen(Qt::NoPen);
        painter.setBrush(colour);
        if (m_orientation == MdStepsOrientation::Horizontal) {
            const QPointF a = centres.at(i) + QPointF(radius + kConnectorGap, 0);
            const QPointF b = centres.at(i + 1) - QPointF(radius + kConnectorGap, 0);
            const QRectF bar(a.x(), a.y() - kConnectorThickness / 2.0,
                             qMax(0.0, b.x() - a.x()), kConnectorThickness);
            painter.drawRect(bar);
        } else {
            const QPointF a = centres.at(i) + QPointF(0, radius + kConnectorGap);
            const QPointF b = centres.at(i + 1) - QPointF(0, radius + kConnectorGap);
            const QRectF bar(a.x() - kConnectorThickness / 2.0, a.y(),
                             kConnectorThickness, qMax(0.0, b.y() - a.y()));
            painter.drawRect(bar);
        }
    }

    for (int i = 0; i < centres.size(); ++i) {
        const StepState state = stateAt(i);
        const QPointF centre = centres.at(i);
        const QRectF disc(centre.x() - radius, centre.y() - radius,
                          radius * 2.0, radius * 2.0);

        painter.setPen(Qt::NoPen);
        painter.setBrush(resolvedDiscColor(state));
        painter.drawEllipse(disc);

        if (state == StepState::Complete) {
            // The check glyph is the completed progress mark. Drawn with the
            // Material Symbols font when available, else a painted polyline
            // so the state still reads under the offscreen font-less platforms.
            const QRectF iconRect(centre.x() - 12.0, centre.y() - 12.0, 24.0, 24.0);
            const QColor content = resolvedDiscContent(state);
            if (!MdIcon::paint(&painter, iconRect, QStringLiteral("check"), content)) {
                QPen pen(content);
                pen.setWidthF(2.0);
                pen.setCapStyle(Qt::RoundCap);
                pen.setJoinStyle(Qt::RoundJoin);
                painter.setPen(pen);
                painter.setBrush(Qt::NoBrush);
                QPainterPath check;
                check.moveTo(centre.x() - 6.0, centre.y());
                check.lineTo(centre.x() - 1.5, centre.y() + 4.5);
                check.lineTo(centre.x() + 6.5, centre.y() - 5.0);
                painter.drawPath(check);
            }
        } else {
            // 1-based step number, the common stepper reading.
            QFont numberFont = MdTypeScale::font(TypeStyle::LabelLarge);
            painter.setFont(numberFont);
            painter.setPen(resolvedDiscContent(state));
            painter.drawText(disc, Qt::AlignCenter, QString::number(i + 1));
        }

        if (i < m_labels.size() && !m_labels.at(i).isEmpty()) {
            painter.setFont(labelFont);
            painter.setPen(resolvedLabelColor(state));
            const QString &text = m_labels.at(i);
            if (m_orientation == MdStepsOrientation::Horizontal) {
                const qreal labelWidth = qMin(qreal(metrics.horizontalAdvance(text)),
                                              width() / qreal(m_stepCount));
                const QRectF labelRect(centre.x() - labelWidth / 2.0,
                                       disc.bottom() + kLabelGap,
                                       labelWidth,
                                       metrics.height());
                painter.drawText(labelRect, Qt::AlignHCenter | Qt::AlignTop, text);
            } else {
                const QRectF labelRect(disc.right() + kLabelGap,
                                       centre.y() - metrics.height() / 2.0,
                                       width() - disc.right() - kLabelGap - kFocusMargin,
                                       metrics.height());
                painter.drawText(labelRect, Qt::AlignLeft | Qt::AlignVCenter, text);
            }
        }
    }

    // :focus-visible ring — keyboard focus only.
    if (hasFocus() && m_focusIsKeyboard) {
        const QRectF bounds = QRectF(rect()).adjusted(kFocusMargin, kFocusMargin,
                                                      -kFocusMargin, -kFocusMargin);
        MdFocusRingSpec spec;
        spec.shape = ShapeCorner::ExtraLarge;
        spec.inward = true;
        const QColor ring = MdTheme::instance().color(ColorRole::Secondary);
        MdFocusRing::paint(&painter, bounds, QList<qreal>(), ring, spec);
    }
}

void MdSteps::mousePressEvent(QMouseEvent *event)
{
    QWidget::mousePressEvent(event);
    if (m_readOnly) {
        return;
    }
    const int index = stepAtPosition(mousePosition(event));
    if (index >= 0) {
        selectStep(index);
    }
}

void MdSteps::keyPressEvent(QKeyEvent *event)
{
    if (!m_readOnly) {
        const bool forward = m_orientation == MdStepsOrientation::Horizontal
                                 ? event->key() == Qt::Key_Right
                                 : event->key() == Qt::Key_Down;
        const bool backward = m_orientation == MdStepsOrientation::Horizontal
                                  ? event->key() == Qt::Key_Left
                                  : event->key() == Qt::Key_Up;
        if (forward) {
            selectStep(m_currentStep + 1);
            return;
        }
        if (backward) {
            selectStep(m_currentStep - 1);
            return;
        }
        if (event->key() == Qt::Key_Home) {
            selectStep(0);
            return;
        }
        if (event->key() == Qt::Key_End) {
            selectStep(m_stepCount - 1);
            return;
        }
    }
    QWidget::keyPressEvent(event);
}

void MdSteps::focusInEvent(QFocusEvent *event)
{
    // Reason-classification matches MdIconButton: Qt::TabFocusReason and
    // Qt::BacktabFocusReason are keyboard; MouseFocusReason is not.
    m_focusIsKeyboard = event->reason() == Qt::TabFocusReason
                        || event->reason() == Qt::BacktabFocusReason
                        || event->reason() == Qt::ShortcutFocusReason;
    QWidget::focusInEvent(event);
    update();
}

void MdSteps::focusOutEvent(QFocusEvent *event)
{
    m_focusIsKeyboard = false;
    QWidget::focusOutEvent(event);
    update();
}

void MdSteps::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::FontChange || event->type() == QEvent::EnabledChange
        || event->type() == QEvent::LayoutDirectionChange) {
        update();
    }
}

void MdSteps::onThemeChanged()
{
    update();
}

} // namespace md
