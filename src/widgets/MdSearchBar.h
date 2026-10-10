#ifndef MD_SEARCH_BAR_H
#define MD_SEARCH_BAR_H

// MdSearchBar — the MD3 search bar, `md.comp.search-bar.*` (34.0.21).
//
// The 56 px entry-point bar: surface-container-high at level 3 with
// corner-full, a 24 px leading search icon, the body-large input line and the
// trailing action slot. Clicking the bar emits `activated()` — the host
// expands it into the search view (the view's surfaces are `MdSearchView`'s).
//
// Keyboard is QLineEdit's: the text edits normally, Enter emits
// `searchRequested(text)`.

#include "core/MdSearchTokens.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtWidgets/QLineEdit>

namespace md {

class MdSearchView;

class QT_MD3_EXPORT MdSearchBar : public QLineEdit
{
    Q_OBJECT

    Q_PROPERTY(QString placeholderText READ placeholderText WRITE setPlaceholderText NOTIFY
                   placeholderTextChanged)
    Q_PROPERTY(md::MdSearchSurface surface READ surface WRITE setSurface NOTIFY surfaceChanged)

public:
    explicit MdSearchBar(QWidget *parent = nullptr);
    ~MdSearchBar() override;

    QString placeholderText() const { return m_placeholder; }
    void setPlaceholderText(const QString &text);

    /// The surface this bar paints: Bar (default), DockedView or FullScreenView.
    MdSearchSurface surface() const { return m_surface; }
    void setSurface(MdSearchSurface surface);

    // --- geometry -----------------------------------------------------------
    /// The 24 px leading icon's rect.
    QRectF leadingIconRect() const;
    /// The container rect — the 56 px pill.
    QRectF containerRect() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    // --- interaction state -------------------------------------------------------
    bool isHovered() const { return m_hovered; }
    bool isEffectivelyDisabled() const { return !isEnabled(); }

    const MdSearchTokens &searchTokens() const;
    void setSearchTokens(const MdSearchTokens &tokens);

    MdComponentTokens &componentTokens()
    {
        m_tokensDirty = true;
        return m_componentTokens;
    }
    const MdComponentTokens &componentTokens() const { return m_componentTokens; }

signals:
    void placeholderTextChanged(const QString &text);
    void surfaceChanged(md::MdSearchSurface surface);
    /// The bar was activated — the host should expand it into the view.
    void activated();
    void searchRequested(const QString &text);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    QString m_placeholder;
    MdSearchSurface m_surface = MdSearchSurface::Bar;
    bool m_hovered = false;

    mutable MdSearchTokens m_tokens;
    mutable bool m_tokensDirty = true;
    MdComponentTokens m_componentTokens;
};

} // namespace md

#endif // MD_SEARCH_BAR_H
