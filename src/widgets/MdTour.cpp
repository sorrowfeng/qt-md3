#include "MdTour.h"

#include "MdButton.h"
#include "core/MdShape.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "styles/MdStyleBase.h"

#include <QtGui/QFontMetrics>
#include <QtGui/QKeyEvent>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>

namespace md {

namespace {

/// Scrim opacity — the modal navigation drawer's carried row (0.32).
constexpr qreal kScrimOpacity = 0.32;
/// Gap between the shape hole and the callout card.
constexpr qreal kCardGap = 16.0;
/// Shape margin around the target rect, in logical px.
constexpr qreal kHoleMargin = 8.0;
constexpr qreal kCardPad = 24.0;
constexpr qreal kButtonHeight = 32.0;
constexpr qreal kButtonGap = 8.0;
constexpr qreal kCardWidth = 320.0;

} // namespace

// --- MdCoachMark ------------------------------------------------------------

MdCoachMark::MdCoachMark(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdCoachMark::~MdCoachMark() = default;

void MdCoachMark::init()
{
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAccessibleName(QStringLiteral("coach mark"));
    setAttribute(Qt::WA_TranslucentBackground, true);

    m_skipButton = new MdButton(this);
    m_skipButton->setVariant(ButtonVariant::Text);
    m_skipButton->setButtonSize(ButtonSize::XSmall);
    m_skipButton->setText(QStringLiteral("Skip"));
    m_skipButton->setObjectName(QStringLiteral("md-coach-skip"));
    connect(m_skipButton, &MdButton::clicked, this, &MdCoachMark::skipRequested);

    m_nextButton = new MdButton(this);
    m_nextButton->setVariant(ButtonVariant::Filled);
    m_nextButton->setButtonSize(ButtonSize::XSmall);
    m_nextButton->setText(QStringLiteral("Next"));
    m_nextButton->setObjectName(QStringLiteral("md-coach-next"));
    connect(m_nextButton, &MdButton::clicked, this, &MdCoachMark::nextRequested);

    MdStyleBase::connectThemeUpdate(this, &MdCoachMark::onThemeChanged);
}

void MdCoachMark::setTitle(const QString &title)
{
    if (m_title == title) {
        return;
    }
    m_title = title;
    updateGeometry();
    update();
    emit titleChanged(m_title);
}

void MdCoachMark::setBody(const QString &body)
{
    if (m_body == body) {
        return;
    }
    m_body = body;
    updateGeometry();
    update();
    emit bodyChanged(m_body);
}

void MdCoachMark::setTargetRect(const QRect &rect)
{
    if (m_targetRect == rect) {
        return;
    }
    m_targetRect = rect;
    layoutButtons();
    update();
    emit targetRectChanged(m_targetRect);
}

void MdCoachMark::setStepText(const QString &text)
{
    if (m_stepText == text) {
        return;
    }
    m_stepText = text;
    update();
    emit stepTextChanged(m_stepText);
}

QRectF MdCoachMark::holeRect() const
{
    return m_targetRect.adjusted(-kHoleMargin, -kHoleMargin, kHoleMargin, kHoleMargin);
}

QRectF MdCoachMark::cardRect() const
{
    const QRectF hole = holeRect();
    qreal y = hole.isValid() && !hole.isNull() ? hole.bottom() + kCardGap : kCardGap;
    // Keep the card on-screen when the hole is near the bottom.
    if (y + 160 > height()) {
        y = qMax(kCardGap, (hole.top() - kCardGap - 160));
    }
    const qreal x = qBound(kCardGap, hole.left(), width() - kCardWidth - kCardGap);
    return QRectF(x, y, kCardWidth, 160);
}

QSize MdCoachMark::sizeHint() const
{
    return QSize(640, 480);
}

int MdCoachMark::heightForWidth(int width) const
{
    Q_UNUSED(width);
    return 480;
}

void MdCoachMark::layoutButtons()
{
    const QRectF card = cardRect();
    const qreal y = card.bottom() - kCardPad - kButtonHeight;
    qreal x = card.right() - kCardPad;
    if (m_nextButton) {
        const QSize hint = m_nextButton->sizeHint();
        x -= hint.width();
        m_nextButton->setGeometry(qRound(x), qRound(y), hint.width(), hint.height());
        x -= kButtonGap;
    }
    if (m_skipButton) {
        const QSize hint = m_skipButton->sizeHint();
        x -= hint.width();
        m_skipButton->setGeometry(qRound(x), qRound(y), hint.width(), hint.height());
    }
}

void MdCoachMark::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Scrim with the shape hole punched out.
    QPainterPath scrimPath;
    scrimPath.addRect(QRectF(0, 0, width(), height()));
    const QRectF hole = holeRect();
    if (hole.isValid() && !hole.isNull() && hole.intersects(rect())) {
        QPainterPath holePath;
        holePath.addRoundedRect(hole, 16, 16);
        scrimPath = scrimPath.subtracted(holePath);
    }
    QColor scrim = MdTheme::instance().color(ColorRole::Scrim);
    scrim.setAlphaF(kScrimOpacity);
    painter.fillPath(scrimPath, scrim);

    // Callout card — the dialog chrome.
    const QRectF card = cardRect();
    const QPainterPath cardPath = MdShape::roundedRect(card, ShapeCorner::ExtraLarge);
    painter.fillPath(cardPath, MdTheme::instance().color(ColorRole::SurfaceContainerHigh));

    qreal y = card.top() + kCardPad;
    painter.setFont(MdTypeScale::font(TypeStyle::HeadlineSmall));
    painter.setPen(MdTheme::instance().color(ColorRole::OnSurface));
    const qreal titleH = QFontMetrics(MdTypeScale::font(TypeStyle::HeadlineSmall)).height();
    painter.drawText(QRectF(card.left() + kCardPad, y, card.width() - 2 * kCardPad, titleH),
                     Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine, m_title);
    y += titleH + 12;

    painter.setFont(MdTypeScale::font(TypeStyle::BodyMedium));
    painter.setPen(MdTheme::instance().color(ColorRole::OnSurfaceVariant));
    const qreal bodyTop = y;
    const qreal bodyH = card.bottom() - kCardPad - kButtonHeight - 12 - bodyTop;
    painter.drawText(QRectF(card.left() + kCardPad, bodyTop, card.width() - 2 * kCardPad, bodyH),
                     Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap, m_body);

    if (!m_stepText.isEmpty()) {
        painter.setFont(MdTypeScale::font(TypeStyle::LabelSmall));
        painter.setPen(MdTheme::instance().color(ColorRole::OnSurfaceVariant));
        painter.drawText(QRectF(card.left() + kCardPad, card.bottom() - kCardPad - kButtonHeight,
                                card.width() - 2 * kCardPad, kButtonHeight),
                         Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine, m_stepText);
    }
}

void MdCoachMark::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    layoutButtons();
}

void MdCoachMark::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        emit skipRequested();
        return;
    }
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter
        || event->key() == Qt::Key_Right) {
        emit nextRequested();
        return;
    }
    QWidget::keyPressEvent(event);
}

void MdCoachMark::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::FontChange || event->type() == QEvent::LayoutDirectionChange) {
        updateGeometry();
        layoutButtons();
        update();
    }
    QWidget::changeEvent(event);
}

void MdCoachMark::onThemeChanged()
{
    update();
}

// --- MdTour -----------------------------------------------------------------

MdTour::MdTour(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdTour::~MdTour() = default;

void MdTour::init()
{
    setFocusPolicy(Qt::NoFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setAccessibleName(QStringLiteral("tour"));
    // The controller paints nothing; the coach mark does.
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    MdStyleBase::connectThemeUpdate(this, &MdTour::onThemeChanged);
}

void MdTour::setSteps(const QList<MdTourStep> &steps)
{
    m_steps = steps;
    m_currentStep = qBound(0, m_currentStep, qMax(0, int(m_steps.size()) - 1));
    emit stepCountChanged(stepCount());
    emit currentStepChanged(m_currentStep);
    if (m_active) {
        applyStep();
    }
}

void MdTour::addStep(const MdTourStep &step)
{
    m_steps.append(step);
    emit stepCountChanged(stepCount());
}

void MdTour::clearSteps()
{
    m_steps.clear();
    m_currentStep = 0;
    emit stepCountChanged(0);
    emit currentStepChanged(0);
}

void MdTour::start()
{
    if (m_steps.isEmpty() || m_active) {
        return;
    }
    m_active = true;
    m_currentStep = 0;
    if (!m_coach) {
        m_coach = new MdCoachMark(parentWidget() ? parentWidget() : this);
        connect(m_coach, &MdCoachMark::nextRequested, this, &MdTour::onNext);
        connect(m_coach, &MdCoachMark::skipRequested, this, &MdTour::onSkip);
    }
    if (m_coach->parent() != parentWidget() && parentWidget()) {
        m_coach->setParent(parentWidget());
    }
    m_coach->setGeometry(parentWidget() ? parentWidget()->rect() : rect());
    m_coach->show();
    m_coach->raise();
    m_coach->setFocus();
    applyStep();
    emit activeChanged(true);
    emit currentStepChanged(m_currentStep);
}

void MdTour::stop()
{
    if (!m_active) {
        return;
    }
    m_active = false;
    if (m_coach) {
        m_coach->hide();
    }
    emit activeChanged(false);
}

void MdTour::next()
{
    if (m_currentStep + 1 >= stepCount()) {
        stop();
        emit finished();
        return;
    }
    ++m_currentStep;
    applyStep();
    emit currentStepChanged(m_currentStep);
}

void MdTour::previous()
{
    if (m_currentStep <= 0) {
        return;
    }
    --m_currentStep;
    applyStep();
    emit currentStepChanged(m_currentStep);
}

void MdTour::onNext()
{
    next();
}

void MdTour::onSkip()
{
    stop();
    emit skipped();
}

QRect MdTour::resolveTarget(const MdTourStep &step) const
{
    if (!step.targetName.isEmpty()) {
        QWidget *host = parentWidget() ? parentWidget() : nullptr;
        if (host) {
            QObject *found = host->findChild<QObject *>(step.targetName);
            if (auto *widget = qobject_cast<QWidget *>(found)) {
                const QPoint topLeft = host->mapFromGlobal(widget->mapToGlobal(QPoint(0, 0)));
                return QRect(topLeft, widget->size());
            }
        }
    }
    return step.targetRect;
}

void MdTour::applyStep()
{
    if (!m_coach || m_currentStep < 0 || m_currentStep >= int(m_steps.size())) {
        return;
    }
    const MdTourStep &step = m_steps.at(m_currentStep);
    m_coach->setTitle(step.title);
    m_coach->setBody(step.body);
    m_coach->setTargetRect(resolveTarget(step));
    m_coach->setStepText(QStringLiteral("%1 / %2").arg(m_currentStep + 1).arg(stepCount()));
    if (m_coach->nextButton()) {
        m_coach->nextButton()->setText(m_currentStep + 1 >= stepCount()
                                           ? QStringLiteral("Done")
                                           : QStringLiteral("Next"));
    }
}

void MdTour::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (m_coach) {
        m_coach->setGeometry(rect());
    }
}

void MdTour::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LayoutDirectionChange) {
        if (m_coach) {
            m_coach->update();
        }
    }
    QWidget::changeEvent(event);
}

void MdTour::onThemeChanged()
{
    if (m_coach) {
        m_coach->update();
    }
}

} // namespace md
