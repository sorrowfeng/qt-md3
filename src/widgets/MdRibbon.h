#ifndef MD_RIBBON_H
#define MD_RIBBON_H

// MdRibbon — Stage 2 Qt extension (Appendix D, P3).
//
// Semantic anchor (appendix D): "Toolbar + Tabs 组合".
//
// The office-style ribbon: a tab strip over a command band. Both halves are
// the Stage 1 exports composed, not redrawn:
//
//   tabs     `MdTabs` (secondary family) selecting the active group set
//   band     a toolbar-height surface-container band holding groups of
//            `MdButton` / `MdIconButton` children, split by outline-variant
//            group dividers
//
// MD3 has no ribbon family; the widget is the toolbar + tabs composition
// appendix D names. The band height follows the toolbar tokens (the docked
// toolbar's expanded height).
//
// There is no QSS.

#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QList>
#include <QtCore/QStringList>
#include <QtWidgets/QWidget>

class QPaintEvent;

namespace md {

class MdTabs;

/// One ribbon tab and the group titles under it.
struct QT_MD3_EXPORT MdRibbonTab
{
    QString title;
    QStringList groups;

    MdRibbonTab() = default;
    explicit MdRibbonTab(const QString &title_, const QStringList &groups_ = QStringList())
        : title(title_)
        , groups(groups_)
    {
    }

    bool operator==(const MdRibbonTab &other) const
    {
        return title == other.title && groups == other.groups;
    }
};

class QT_MD3_EXPORT MdRibbon : public QWidget
{
    Q_OBJECT

    /// The ribbon's tabs.
    Q_PROPERTY(QList<md::MdRibbonTab> tabs READ tabs WRITE setTabs NOTIFY tabsChanged)
    /// Index of the active tab.
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    /// Height of the command band, in logical px (default 88, the docked
    /// toolbar's expanded height).
    Q_PROPERTY(int bandHeight READ bandHeight WRITE setBandHeight NOTIFY bandHeightChanged)

public:
    explicit MdRibbon(QWidget *parent = nullptr);
    ~MdRibbon() override;

    QList<MdRibbonTab> tabs() const { return m_tabs; }
    void setTabs(const QList<MdRibbonTab> &tabs);
    void addTab(const MdRibbonTab &tab);
    void clearTabs();

    int currentIndex() const { return m_currentIndex; }
    void setCurrentIndex(int index);

    int bandHeight() const { return m_bandHeight; }
    void setBandHeight(int height);

    int count() const { return int(m_tabs.size()); }
    MdRibbonTab tabAt(int index) const;

    /// The underlying tabs strip (the Stage 1 MdTabs).
    MdTabs *tabStrip() const { return m_tabStrip; }

    /// Add a command widget into the band under `groupIndex` of the current
    /// tab. The widget is reparented into the ribbon.
    void addCommand(QWidget *command, int groupIndex = 0);
    void clearCommands();

    /// Resolved band background.
    QColor resolvedBandColor() const;
    /// Resolved group-divider colour.
    QColor resolvedDividerColor() const;

    /// Height of the tabs strip (the MdTabs size hint).
    qreal tabStripHeight() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void tabsChanged(const QList<md::MdRibbonTab> &tabs);
    void currentIndexChanged(int index);
    void bandHeightChanged(int height);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onThemeChanged();
    void onTabChanged(int index);

private:
    void init();
    void rebuildTabStrip();
    void layoutBand();

    QList<MdRibbonTab> m_tabs;
    int m_currentIndex = -1;
    int m_bandHeight = 88;
    MdTabs *m_tabStrip = nullptr;
    QList<QList<QWidget *>> m_commands; // per group of the current tab
};

} // namespace md

Q_DECLARE_METATYPE(md::MdRibbonTab)
Q_DECLARE_METATYPE(QList<md::MdRibbonTab>)

#endif // MD_RIBBON_H
