#ifndef MD_BOTTOM_APP_BAR_H
#define MD_BOTTOM_APP_BAR_H

// MdBottomAppBar — the MD3 bottom app bar.
//
// "A bottom app bar displays navigation and key actions at the bottom of small
// screens." It is the second half of the official *App bars* component, and it
// shares nothing with the top app bar but the container colour family: the
// layout is one row, the height is a single 80 px, and the elevation is a flat
// level 2 rather than a level 0/level 2 pair.
//
// Two rows the export still publishes are deprecated and deliberately unused:
// `with-fab.container.height` (72 px), because "bottom app bar design updated
// to use a single height for all configurations, with vertically centered
// content", and `container.surface-tint-layer.color`, deprecated with the move
// from opacity-based to tonal surfaces. A theme that sets either still
// resolves; neither changes a pixel. Both are recorded in
// docs/porting-todo.md.
//
// A docked floating action button is a slot, not a separate component: the
// caller hands over any widget and the bar places it against its own trailing
// strip, which is where Compose's `padding(top = 8, end = 12)` on top of the
// container's own 4 px puts a FAB 16 px from the edge and 12 px from the top.
//
// `FlexibleBottomAppBar` is **not** this component. It reads
// `DockedToolbarTokens` and is the docked toolbar that the ★ Toolbars family
// will port; its configurable spacing and 16 px container padding belong
// there, not here.

#include "core/MdAppBarTokens.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QList>
#include <QtWidgets/QWidget>

namespace md {

class MdBottomAppBarStyle;

class QT_MD3_EXPORT MdBottomAppBar : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(Arrangement arrangement READ arrangement WRITE setArrangement NOTIFY
                   arrangementChanged)

public:
    /// How the row's children are distributed across the content band.
    ///
    /// Compose passes `Arrangement.Start` for the plain bottom app bar, and
    /// that is the default. The other four are the published
    /// `Arrangement.Horizontal` set the layout function accepts, offered
    /// because they cost nothing and a "spaced between" bottom bar is a
    /// documented pattern.
    enum class Arrangement
    {
        Start,
        Center,
        End,
        SpaceBetween,
        Count,
    };
    Q_ENUM(Arrangement)

    explicit MdBottomAppBar(QWidget *parent = nullptr);
    ~MdBottomAppBar() override;

    Arrangement arrangement() const { return m_arrangement; }
    void setArrangement(Arrangement arrangement);

    /// The row's children, in order from the leading edge. A docked FAB is
    /// *not* part of this list: it has its own trailing strip and is placed
    /// after them, which is why `arrangement` never moves it.
    void addWidget(QWidget *widget);
    void insertWidget(int index, QWidget *widget);
    void removeWidget(QWidget *widget);
    void clearWidgets();
    QList<QWidget *> widgets() const { return m_widgets; }

    /// The docked floating action button, or null.
    QWidget *floatingActionButton() const { return m_fab; }
    void setFloatingActionButton(QWidget *widget);

    // --- tokens -------------------------------------------------------------
    const MdBottomAppBarTokens &tokens() const;

    MdComponentTokens &componentTokens()
    {
        m_tokensDirty = true;
        return m_componentTokens;
    }
    const MdComponentTokens &componentTokens() const { return m_componentTokens; }

    // --- geometry -----------------------------------------------------------
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void arrangementChanged(Arrangement arrangement);
    void widgetsChanged();
    void floatingActionButtonChanged();

protected:
    void resizeEvent(QResizeEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onThemeChanged();

private:
    void init();
    void invalidateTokens();
    void placeChildren();

    Arrangement m_arrangement = Arrangement::Start;
    QList<QWidget *> m_widgets;
    QWidget *m_fab = nullptr;

    mutable MdBottomAppBarTokens m_tokens;
    mutable bool m_tokensDirty = true;
    MdComponentTokens m_componentTokens;
};

} // namespace md

#endif // MD_BOTTOM_APP_BAR_H
