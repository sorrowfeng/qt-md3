#include "MdDatePickerStyle.h"

#include "core/MdElevation.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "widgets/MdDatePicker.h"

#include <QtCore/QMutex>
#include <QtCore/QMutexLocker>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>

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

} // namespace

MdDatePickerStyle::MdDatePickerStyle(QObject *parent)
    : MdStyleBase(parent)
{
}

MdDatePickerStyle *MdDatePickerStyle::shared()
{
    static QMutex mutex;
    static MdDatePickerStyle *instance = nullptr;

    QMutexLocker locker(&mutex);
    if (!instance) {
        // Intentionally never destroyed: a paint filter must outlive every
        // widget it paints.
        instance = new MdDatePickerStyle;
        installPaintFilter<MdDatePicker>(instance);
    }
    return instance;
}

bool MdDatePickerStyle::isInstalled()
{
    return hasPaintFilter(&MdDatePicker::staticMetaObject);
}

void MdDatePickerStyle::paintDatePicker(QPainter &painter, const MdDatePicker &picker,
                                        const MdDatePickerTokens &tokens)
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

    // 2 — the header.
    {
        const QRectF header = picker.headerRect();
        const QFont headlineFont = MdTypeScale::font(TypeStyle::HeadlineLarge);
        painter.setFont(headlineFont);
        painter.setPen(MdTheme::instance().color(tokens.headerHeadlineColor));
        painter.drawText(header.adjusted(16.0, 8.0, -16.0, -40.0), MdStyleBase::leadingAlignment(&picker) | Qt::AlignVCenter,
                         picker.selectedDate().isValid()
                             ? picker.selectedDate().toString(QStringLiteral("ddd, MMM d"))
                             : QStringLiteral("Select date"));

        const QFont supportingFont = MdTypeScale::font(TypeStyle::LabelLarge);
        painter.setFont(supportingFont);
        painter.setPen(MdTheme::instance().color(tokens.headerSupportingColor));
        painter.drawText(header.adjusted(16.0, header.height() - 48.0, -16.0, -8.0),
                         MdStyleBase::leadingAlignment(&picker) | Qt::AlignVCenter,
                         picker.displayedMonth().toString(QStringLiteral("MMMM yyyy")));
    }

    if (picker.face() == MdDatePickerFace::Calendar) {
        // 3 — the weekday row.
        {
            const QFont weekdayFont = MdTypeScale::font(TypeStyle::BodyLarge);
            painter.setFont(weekdayFont);
            painter.setPen(MdTheme::instance().color(tokens.weekdayColor));
            static const char *kDays[7] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
            for (int col = 0; col < 7; ++col) {
                const QRectF cell = picker.dateCellRect(-1, col);
                painter.drawText(QRectF(cell.left(), picker.headerRect().bottom() + 4.0,
                                        cell.width(), 20.0),
                                 Qt::AlignCenter, QLatin1String(kDays[col]));
            }
        }

        // 4 — the month subhead.
        {
            const QFont subheadFont = MdTypeScale::font(TypeStyle::TitleSmall);
            painter.setFont(subheadFont);
            painter.setPen(MdTheme::instance().color(tokens.monthSubheadColor));
            painter.drawText(QRectF(picker.headerRect().left(), picker.headerRect().bottom() + 24.0,
                                    picker.headerRect().width(), 20.0),
                             MdStyleBase::leadingAlignment(&picker) | Qt::AlignVCenter,
                             picker.displayedMonth().toString(QStringLiteral("MMMM yyyy")));
        }

        // 5 — the calendar grid. The style resolves each slot through the
        // picker's public dateAtCell contract — the same helper the hit test
        // uses, so paint and hit can never disagree.
        const QDate today = QDate::currentDate();
        for (int row = 0; row < 6; ++row) {
            for (int col = 0; col < 7; ++col) {
                const QRectF cell = picker.dateCellRect(row, col);
                const QDate date = picker.dateAtCell(row, col);
                if (!date.isValid()) {
                    continue;
                }
                const bool isSelected = date == picker.selectedDate();
                const bool isToday = date == today;
                const bool inRange = picker.isRange() && picker.rangeStart().isValid()
                    && picker.rangeEnd().isValid() && date >= picker.rangeStart()
                    && date <= picker.rangeEnd();

                if (inRange && !isSelected) {
                    painter.setPen(Qt::NoPen);
                    painter.setBrush(MdTheme::instance().color(tokens.rangeIndicatorColor));
                    painter.drawRoundedRect(cell, tokens.dateCellSize / 2.0,
                                            tokens.dateCellSize / 2.0);
                } else if (isSelected) {
                    painter.setPen(Qt::NoPen);
                    const QColor fill = resolveSlot(
                        tokens.dateSelected[int(MdDatePickerState::Enabled)]);
                    painter.setBrush(fill.isValid() ? fill : QColor());
                    painter.drawRoundedRect(cell, tokens.dateCellSize / 2.0,
                                            tokens.dateCellSize / 2.0);
                } else if (isToday) {
                    painter.setPen(QPen(MdTheme::instance().color(tokens.dateTodayColor),
                                        tokens.dateTodayOutlineWidth));
                    painter.setBrush(Qt::NoBrush);
                    painter.drawRoundedRect(cell, tokens.dateCellSize / 2.0,
                                            tokens.dateCellSize / 2.0);
                }

                const QFont labelFont = MdTypeScale::font(TypeStyle::BodyLarge);
                painter.setFont(labelFont);
                const QColor label = isSelected
                    ? MdTheme::instance().color(ColorRole::OnPrimary)
                    : (inRange ? MdTheme::instance().color(tokens.rangeInLabelColor)
                        : (isToday ? MdTheme::instance().color(tokens.dateTodayColor)
                                   : MdTheme::instance().color(ColorRole::OnSurface)));
                painter.setPen(label);
                painter.drawText(cell, Qt::AlignCenter, QString::number(date.day()));
            }
        }
    } else {
        // 6 — the year list.
        for (int i = 0; i < 12; ++i) {
            const QRectF cell = picker.yearCellRect(i);
            const int year = picker.displayedMonth().year() - 6 + i;
            const bool selected = year == picker.selectedDate().year();
            if (selected) {
                painter.setPen(Qt::NoPen);
                const QColor fill =
                    resolveSlot(tokens.yearSelected[int(MdDatePickerState::Enabled)]);
                painter.setBrush(fill.isValid() ? fill : QColor());
                painter.drawRoundedRect(cell, tokens.yearHeight / 2.0, tokens.yearHeight / 2.0);
            }
            const QFont labelFont = MdTypeScale::font(TypeStyle::BodyLarge);
            painter.setFont(labelFont);
            painter.setPen(MdTheme::instance().color(
                selected ? ColorRole::OnPrimary : ColorRole::OnSurfaceVariant));
            painter.drawText(cell, Qt::AlignCenter, QString::number(year));
        }
    }

    painter.restore();
}

void MdDatePickerStyle::drawWidget(QPainter *painter, QWidget *widget)
{
    MdDatePicker *picker = qobject_cast<MdDatePicker *>(widget);
    if (!picker || !painter) {
        return;
    }
    paintDatePicker(*painter, *picker, picker->datePickerTokens());
}

} // namespace md
