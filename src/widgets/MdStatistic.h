#ifndef MD_STATISTIC_H
#define MD_STATISTIC_H

// MdStatistic — Stage 2 Qt extension (Appendix D, P2).
//
// Semantic anchor (appendix D): "Display / headline typography 展示数值" —
// Display / headline typography used to present a number. MD3 has no
// statistic family, so the widget is composed from the type scale alone:
//
//   value     Display/Headline (configurable, default DisplaySmall)
//   title     LabelLarge above the number (on-surface-variant)
//   prefix    same style as value, drawn before it (on-surface)
//   suffix    same style as value, drawn after it (on-surface-variant)
//   trend     LabelMedium + Material Symbols arrow (up / down / none)
//
// Every type size comes from `MdTypeScale` (`md.sys.typescale.*`). The trend
// colour maps to `primary` when improving and `error` when declining —
// the semantic-colour reading the error family already publishes
// (`md.sys.color.error`). There is no QSS.

#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtWidgets/QWidget>

class QPaintEvent;
class QResizeEvent;

namespace md {

/// Direction of a trend indicator drawn beside the value.
enum class MdStatisticTrend {
    None,
    Up,
    Down,
    Count,
};

class QT_MD3_EXPORT MdStatistic : public QWidget
{
    Q_OBJECT

    /// Numeric value shown. Formatted with `precision` decimals.
    Q_PROPERTY(qreal value READ value WRITE setValue NOTIFY valueChanged)
    /// Decimal places in the formatted value. 0 renders an integer.
    Q_PROPERTY(int precision READ precision WRITE setPrecision NOTIFY precisionChanged)
    /// Optional label above the number (LabelLarge).
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    /// Text drawn before the number, e.g. a currency mark.
    Q_PROPERTY(QString prefix READ prefix WRITE setPrefix NOTIFY prefixChanged)
    /// Text drawn after the number, e.g. a unit.
    Q_PROPERTY(QString suffix READ suffix WRITE setSuffix NOTIFY suffixChanged)
    /// Type style of the value. Display / headline sizes.
    Q_PROPERTY(md::TypeStyle valueStyle READ valueStyle WRITE setValueStyle NOTIFY valueStyleChanged)
    /// Trend indicator next to the number.
    Q_PROPERTY(md::MdStatisticTrend trend READ trend WRITE setTrend NOTIFY trendChanged)
    /// Extra trend caption, e.g. "+12%". Empty hides the caption.
    Q_PROPERTY(QString trendText READ trendText WRITE setTrendText NOTIFY trendTextChanged)

public:
    explicit MdStatistic(QWidget *parent = nullptr);
    ~MdStatistic() override;

    qreal value() const { return m_value; }
    void setValue(qreal value);

    int precision() const { return m_precision; }
    void setPrecision(int precision);

    QString title() const { return m_title; }
    void setTitle(const QString &title);

    QString prefix() const { return m_prefix; }
    void setPrefix(const QString &prefix);

    QString suffix() const { return m_suffix; }
    void setSuffix(const QString &suffix);

    TypeStyle valueStyle() const { return m_valueStyle; }
    void setValueStyle(TypeStyle style);

    MdStatisticTrend trend() const { return m_trend; }
    void setTrend(MdStatisticTrend trend);

    QString trendText() const { return m_trendText; }
    void setTrendText(const QString &text);

    /// Resolved value colour: on-surface.
    QColor resolvedValueColor() const;
    /// Resolved title colour: on-surface-variant.
    QColor resolvedTitleColor() const;
    /// Resolved trend colour: primary when Up, error when Down, on-surface-variant when None.
    QColor resolvedTrendColor() const;

    /// The formatted number, without prefix / suffix.
    QString formattedValue() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void valueChanged(qreal value);
    void precisionChanged(int precision);
    void titleChanged(const QString &title);
    void prefixChanged(const QString &prefix);
    void suffixChanged(const QString &suffix);
    void valueStyleChanged(md::TypeStyle style);
    void trendChanged(md::MdStatisticTrend trend);
    void trendTextChanged(const QString &text);

protected:
    void paintEvent(QPaintEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onThemeChanged();

private:
    void init();
    QSizeF measure() const;

    qreal m_value = 0.0;
    int m_precision = 0;
    QString m_title;
    QString m_prefix;
    QString m_suffix;
    TypeStyle m_valueStyle = TypeStyle::DisplaySmall;
    MdStatisticTrend m_trend = MdStatisticTrend::None;
    QString m_trendText;
};

} // namespace md

Q_DECLARE_METATYPE(md::MdStatisticTrend)

#endif // MD_STATISTIC_H
