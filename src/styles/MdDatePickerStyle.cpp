#include "MdDatePickerStyle.h"

#include "core/MdElevation.h"
#include "core/MdIcon.h"
#include "core/MdShape.h"
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
    // The docked face is a separate token set and a separate header. Dispatch
    // here so every caller paints whichever variant the picker is in.
    if (picker.variant() == MdDatePickerVariant::Docked) {
        paintDatePickerDocked(painter, picker, picker.datePickerDockedTokens());
        return;
    }

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
        // The header's supporting line is the *year* — it is the year-face
        // toggle in M3, not the month. The month lives once, in the calendar
        // subhead below; painting it here too doubled every "June 2026".
        painter.drawText(header.adjusted(16.0, header.height() - 48.0, -16.0, -8.0),
                         MdStyleBase::leadingAlignment(&picker) | Qt::AlignVCenter,
                         picker.face() == MdDatePickerFace::Years
                             ? QStringLiteral("Select year")
                             : QString::number(picker.displayedMonth().year()));
    }

    if (picker.face() == MdDatePickerFace::Calendar) {
        // 3 — the month subhead sits *above* the weekday row (M3's order:
        // month, then Mon…Sun, then the day grid). The first revision parked
        // it under the weekdays and on top of week row 1.
        {
            const QFont subheadFont = MdTypeScale::font(TypeStyle::TitleSmall);
            painter.setFont(subheadFont);
            painter.setPen(MdTheme::instance().color(tokens.monthSubheadColor));
            painter.drawText(QRectF(picker.headerRect().left(), picker.headerRect().bottom() + 4.0,
                                    picker.headerRect().width(), 20.0),
                             MdStyleBase::leadingAlignment(&picker) | Qt::AlignVCenter,
                             picker.displayedMonth().toString(QStringLiteral("MMMM yyyy")));
        }

        // 4 — the weekday row.
        {
            const QFont weekdayFont = MdTypeScale::font(TypeStyle::BodyLarge);
            painter.setFont(weekdayFont);
            painter.setPen(MdTheme::instance().color(tokens.weekdayColor));
            static const char *kDays[7] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
            for (int col = 0; col < 7; ++col) {
                const QRectF cell = picker.dateCellRect(-1, col);
                painter.drawText(QRectF(cell.left(), picker.headerRect().bottom() + 28.0,
                                        cell.width(), 20.0),
                                 Qt::AlignCenter, QLatin1String(kDays[col]));
            }
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

void MdDatePickerStyle::paintDatePickerDocked(QPainter &painter, const MdDatePicker &picker,
                                              const MdDatePickerDockedTokens &tokens)
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF panel(0.0, 0.0, tokens.containerWidth, tokens.containerHeight);

    // 1 — the container: surface-container-high at level 3, corner-large.
    {
        const qreal radius = MdShape::resolvedRadius(tokens.containerShape, panel.size());
        MdElevation::drawShadowDp(&painter, panel, radius, int(tokens.containerElevation),
                                  MdTheme::instance().color(ColorRole::Shadow));
        painter.setPen(Qt::NoPen);
        painter.setBrush(MdTheme::instance().color(tokens.containerColor));
        painter.drawRoundedRect(panel, radius, radius);
    }

    // 2 — the 64 px header's two menu buttons (month / year).
    {
        const auto paintButton = [&](const QRectF &rect, const QString &label) {
            const qreal radius =
                MdShape::resolvedRadius(tokens.menuButtonShape, rect.size());
            painter.setPen(Qt::NoPen);
            painter.setBrush(Qt::NoBrush);
            painter.drawRoundedRect(rect, radius, radius);

            const QRectF labelRect = rect.adjusted(16.0, 0.0, -34.0, 0.0);
            painter.setFont(MdTypeScale::font(tokens.menuButtonLabelType, TypeEmphasis::Baseline,
                                              MdTheme::instance().scriptCategory()));
            painter.setPen(MdTheme::instance().color(ColorRole::OnSurfaceVariant));
            painter.drawText(labelRect, int(Qt::AlignVCenter | MdStyleBase::leadingAlignment(&picker)),
                             label);

            // The 18 px trailing chevron.
            const QRectF iconRect(rect.right() - 16.0 - tokens.menuButtonIconSize,
                                  rect.top() + (rect.height() - tokens.menuButtonIconSize) / 2.0,
                                  tokens.menuButtonIconSize, tokens.menuButtonIconSize);
            MdIcon::paint(&painter, iconRect, QStringLiteral("arrow_drop_down"),
                          MdTheme::instance().color(ColorRole::OnSurfaceVariant),
                          MdIconSet::MaterialSymbols, MdIconFamily::Outlined);
        };
        paintButton(picker.monthMenuButtonRect(),
                    picker.displayedMonth().toString(QStringLiteral("MMMM")));
        paintButton(picker.yearMenuButtonRect(),
                    QString::number(picker.displayedMonth().year()));
    }

    // 3 — the weekday row.
    {
        painter.setFont(MdTypeScale::font(tokens.weekdayType, TypeEmphasis::Baseline,
                                          MdTheme::instance().scriptCategory()));
        painter.setPen(MdTheme::instance().color(tokens.weekdayColor));
        static const char *kDays[7] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
        for (int col = 0; col < 7; ++col) {
            const QRectF cell = picker.dateCellRect(-1, col);
            // The weekday row sits in the 32 px band under the header.
            painter.drawText(QRectF(cell.left(), picker.headerRect().bottom(), cell.width(), 32.0),
                             Qt::AlignCenter, QLatin1String(kDays[col]));
        }
    }

    // 4 — the calendar grid. Each cell is 48×48; the 40×40 corner-full pill
    // inside it is the state layer / selection shape.
    const QDate today = QDate::currentDate();
    for (int row = 0; row < 6; ++row) {
        for (int col = 0; col < 7; ++col) {
            const QRectF cell = picker.dateCellRect(row, col);
            const QRectF pill = picker.dateStateLayerRect(row, col);
            const QDate date = picker.dateAtCell(row, col);
            if (!date.isValid()) {
                continue;
            }
            const bool isSelected = date == picker.selectedDate();
            const bool isToday = date == today;
            const bool inRange = picker.isRange() && picker.rangeStart().isValid()
                && picker.rangeEnd().isValid() && date >= picker.rangeStart()
                && date <= picker.rangeEnd();

            const qreal pillRadius =
                MdShape::resolvedRadius(tokens.dateStateLayerShape, pill.size());
            if (inRange && !isSelected) {
                painter.setPen(Qt::NoPen);
                painter.setBrush(MdTheme::instance().color(ColorRole::SecondaryContainer));
                painter.drawRoundedRect(pill, pillRadius, pillRadius);
            } else if (isSelected) {
                painter.setPen(Qt::NoPen);
                painter.setBrush(MdTheme::instance().color(ColorRole::Primary));
                painter.drawRoundedRect(pill, pillRadius, pillRadius);
            } else if (isToday) {
                painter.setPen(QPen(MdTheme::instance().color(tokens.dateTodayColor),
                                    tokens.dateTodayOutlineWidth));
                painter.setBrush(Qt::NoBrush);
                painter.drawRoundedRect(pill, pillRadius, pillRadius);
            }

            painter.setFont(MdTypeScale::font(tokens.dateLabelType, TypeEmphasis::Baseline,
                                              MdTheme::instance().scriptCategory()));
            QColor label;
            if (isSelected) {
                label = MdTheme::instance().color(ColorRole::OnPrimary);
            } else if (inRange) {
                label = MdTheme::instance().color(ColorRole::OnSecondaryContainer);
            } else if (isToday) {
                label = MdTheme::instance().color(tokens.dateTodayColor);
            } else {
                label = MdTheme::instance().color(ColorRole::OnSurface);
                // Outside-month days fade to 0.38 (`date-unselected-outside-
                // month.label-text.opacity`).
                const QDate first(picker.displayedMonth().year(), picker.displayedMonth().month(),
                                  1);
                if (date.month() != first.month()) {
                    label.setAlphaF(label.alphaF() * tokens.dateOutsideMonthLabelOpacity);
                }
            }
            painter.setPen(label);
            painter.drawText(cell, Qt::AlignCenter, QString::number(date.day()));
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
