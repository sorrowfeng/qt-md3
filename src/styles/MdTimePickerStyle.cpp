#include "MdTimePickerStyle.h"

#include "core/MdElevation.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "widgets/MdTimePicker.h"

#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtMath>

namespace md {

namespace {

QColor resolveSlot(const MdNavigationColourSlot &slot)
{
    if (!slot.isPresent()) {
        return QColor();
    }
    QColor colour = MdTheme::instance().color(slot.role);
    if (!colour.isValid()) {
        return QColor();
    }
    colour.setAlphaF(qBound(0.0, colour.alphaF() * slot.opacity, 1.0));
    return colour;
}

QString hourText(const MdTimePicker &picker)
{
    const QTime time = picker.time();
    if (picker.is24h()) {
        return QString::asprintf("%02d", time.hour());
    }
    const int h = time.hour() % 12 == 0 ? 12 : time.hour() % 12;
    return QString::number(h);
}

QString minuteText(const MdTimePicker &picker)
{
    return QString::asprintf("%02d", picker.time().minute());
}

} // namespace

MdTimePickerStyle::MdTimePickerStyle(QObject *parent)
    : MdStyleBase(parent)
{
}

MdTimePickerStyle *MdTimePickerStyle::shared()
{
    static QMutex mutex;
    static MdTimePickerStyle *instance = nullptr;

    QMutexLocker locker(&mutex);
    if (!instance) {
        // Intentionally never destroyed: a paint filter must outlive every
        // widget it paints.
        instance = new MdTimePickerStyle;
        installPaintFilter<MdTimePicker>(instance);
    }
    return instance;
}

bool MdTimePickerStyle::isInstalled()
{
    return hasPaintFilter(&MdTimePicker::staticMetaObject);
}

void MdTimePickerStyle::paintTimePicker(QPainter &painter, const MdTimePicker &picker,
                                        const MdTimePickerTokens &tokens)
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);

    // 1 — the container.
    {
        const QRectF bounds = QRectF(picker.rect()).adjusted(8.0, 8.0, -8.0, -8.0);
        MdElevation::drawShadowDp(&painter, bounds, tokens.containerRadius,
                                  int(tokens.containerElevation),
                                  MdTheme::instance().color(ColorRole::Shadow));
        painter.setPen(Qt::NoPen);
        painter.setBrush(MdTheme::instance().color(tokens.containerColor));
        painter.drawRoundedRect(bounds, tokens.containerRadius, tokens.containerRadius);
    }

    // 2 — the headline.
    {
        const QFont headlineFont = MdTypeScale::font(TypeStyle::LabelMedium);
        painter.setFont(headlineFont);
        painter.setPen(MdTheme::instance().color(tokens.headlineColor));
        const QRectF headline(picker.rect().left() + 24.0, picker.rect().top() + 12.0,
                              picker.rect().width() - 48.0, 20.0);
        painter.drawText(headline, MdStyleBase::leadingAlignment(&picker) | Qt::AlignVCenter,
                         QStringLiteral("Select time"));
    }

    // 3 — the time selectors.
    const QRectF hours = picker.timeSelectorRect(true);
    const QRectF minutes = picker.timeSelectorRect(false);
    const MdTimePickerFace face = picker.face();
    auto paintSelector = [&](const QRectF &rect, const QString &text, bool selected) {
        const MdNavigationColourSlot &slot =
            selected ? tokens.timeSelected[int(MdTimeSelectorState::Enabled)]
                     : tokens.timeUnselected[int(MdTimeSelectorState::Enabled)];
        const QColor container = resolveSlot(slot);
        if (container.isValid()) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(container);
            painter.drawRoundedRect(rect, tokens.timeSelectorRadius, tokens.timeSelectorRadius);
        }
        const QFont font = MdTypeScale::font(TypeStyle::DisplayLarge);
        painter.setFont(font);
        painter.setPen(MdTheme::instance().color(selected ? ColorRole::OnPrimaryContainer
                                                          : ColorRole::OnSurface));
        painter.drawText(rect, Qt::AlignCenter, text);
    };

    if (picker.is24h()) {
        const QString text = hourText(picker) + QStringLiteral(":") + minuteText(picker);
        paintSelector(picker.timeSelectorRect(true), text, true);
    } else {
        paintSelector(hours, hourText(picker), face == MdTimePickerFace::Hours);
        paintSelector(minutes, minuteText(picker), face == MdTimePickerFace::Minutes);
        // The `:` separator between the two.
        painter.setFont(MdTypeScale::font(TypeStyle::DisplayLarge));
        painter.setPen(MdTheme::instance().color(ColorRole::OnSurface));
        painter.drawText(QRectF(hours.right(), hours.top(), minutes.left() - hours.right(),
                                hours.height()),
                         Qt::AlignCenter, QStringLiteral(":"));
    }

    // 4 — the period selector (12h mode only).
    if (!picker.is24h()) {
        const QRectF am = picker.periodSelectorRect(true);
        const QRectF pm = picker.periodSelectorRect(false);
        const bool isPm = picker.period() == MdTimePeriod::Pm;
        auto paintPeriod = [&](const QRectF &rect, const QString &text, bool selected) {
            const MdNavigationColourSlot &slot =
                selected ? tokens.periodSelected[int(MdTimeSelectorState::Enabled)]
                         : tokens.periodUnselected[int(MdTimeSelectorState::Enabled)];
            const QColor container = resolveSlot(slot);
            painter.setPen(Qt::NoPen);
            if (container.isValid()) {
                painter.setBrush(container);
                painter.drawRoundedRect(rect, tokens.periodRadius, tokens.periodRadius);
            }
            painter.setFont(MdTypeScale::font(TypeStyle::TitleMedium));
            painter.setPen(MdTheme::instance().color(selected ? ColorRole::OnTertiaryContainer
                                                              : ColorRole::OnSurfaceVariant));
            painter.drawText(rect, Qt::AlignCenter, text);
        };
        paintPeriod(am, QStringLiteral("AM"), !isPm);
        paintPeriod(pm, QStringLiteral("PM"), isPm);
        painter.setPen(QPen(MdTheme::instance().color(tokens.periodOutlineColor),
                            tokens.periodOutlineWidth));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(am.united(pm), tokens.periodRadius, tokens.periodRadius);
    }

    // 5 — the clock dial.
    {
        const QRectF dial = picker.dialRect();
        painter.setPen(Qt::NoPen);
        painter.setBrush(MdTheme::instance().color(tokens.dialColor));
        painter.drawEllipse(dial);

        const QRectF handle = picker.selectorHandleRect();
        // The 2 px track from the centre to the handle.
        painter.setPen(QPen(MdTheme::instance().color(tokens.selectorColor),
                            tokens.selectorTrackWidth));
        painter.drawLine(dial.center(), handle.center());

        // The 48 px handle.
        painter.setPen(Qt::NoPen);
        painter.setBrush(MdTheme::instance().color(tokens.selectorColor));
        painter.drawEllipse(handle);

        // The 8 px centre dot.
        painter.drawEllipse(dial.center(), tokens.selectorCenterSize / 2.0,
                            tokens.selectorCenterSize / 2.0);

        // The numbers around the rim.
        const QFont labelFont = MdTypeScale::font(TypeStyle::BodyLarge);
        painter.setFont(labelFont);
        const int slotCount = picker.is24h() ? 24 : 12;
        const qreal radius = dial.width() / 2.0 - 24.0;
        for (int i = 0; i < slotCount; ++i) {
            const qreal angle = qDegreesToRadians(-90.0 + 360.0 * i / slotCount);
            const QPointF pos = dial.center() + QPointF(qCos(angle) * radius, qSin(angle) * radius);
            const bool selected = picker.face() == MdTimePickerFace::Hours
                && ((picker.is24h() && picker.time().hour() == i)
                    || (!picker.is24h()
                        && (picker.time().hour() % 12 == 0 ? 12 : picker.time().hour() % 12)
                            == (i == 0 ? 12 : i)));
            painter.setPen(MdTheme::instance().color(selected ? tokens.dialSelectedLabel
                                                              : tokens.dialUnselectedLabel));
            const QRectF cell(pos.x() - 16.0, pos.y() - 16.0, 32.0, 32.0);
            const int label = picker.face() == MdTimePickerFace::Minutes ? i * 5
                             : (picker.is24h() ? i : (i == 0 ? 12 : i));
            painter.drawText(cell, Qt::AlignCenter, QString::number(label));
        }
    }

    painter.restore();
}

void MdTimePickerStyle::drawWidget(QPainter *painter, QWidget *widget)
{
    MdTimePicker *picker = qobject_cast<MdTimePicker *>(widget);
    if (!picker || !painter) {
        return;
    }
    paintTimePicker(*painter, *picker, picker->timePickerTokens());
}

} // namespace md
