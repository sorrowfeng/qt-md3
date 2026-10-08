#ifndef MD_BADGE_H
#define MD_BADGE_H

// MdBadge — MD3 badges, the first Communication family.
//
// One token set, two forms, decided by content — not by a variant:
//
//   dot     md.comp.badge.*         a 6 px circle, no content
//   content md.comp.badge.large.*   a pill, minimum 16 px, short text
//                                   (a count, "New", …) in label-small
//
// A badge is *not interactive*: the export publishes no state rows at all.
// It takes no focus, paints no state layer, no ripple, no focus indicator,
// and it does not react to hover or press — see the token header and
// docs/porting-todo.md. This is the first Stage 1 family whose entire
// interaction contract is "nothing happens", and MdBadgedBox relies on it:
// the badge lets clicks pass through to whatever sits underneath
// (Compose: "Not using Surface composable because it blocks touch
// propagation behind it" [compose]).
//
// A standalone MdBadge is the component itself; MdBadgedBox is the anchor
// container that positions a badge over arbitrary content with the
// published offsets. Badges announce their text as the accessible name —
// a count the eye can read must reach the screen reader too.

#include "core/MdBadgeTokens.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QString>
#include <QtWidgets/QWidget>

namespace md {

class MdBadgeStyle;

class QT_MD3_EXPORT MdBadge : public QWidget
{
    Q_OBJECT

    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)

public:
    explicit MdBadge(QWidget *parent = nullptr);
    /// Empty `text` gives the dot form; any text gives the content form.
    explicit MdBadge(const QString &text, QWidget *parent = nullptr);
    ~MdBadge() override;

    // --- content -----------------------------------------------------------
    QString text() const { return m_text; }
    void setText(const QString &text);

    /// True when the badge shows text — the content (large) form.
    bool hasContent() const { return !m_text.isEmpty(); }

    // --- tokens ------------------------------------------------------------
    /// The resolved `md.comp.badge.*` set for this badge's form, after the
    /// application-wide and per-instance `md.comp.*` overrides.
    const MdBadgeTokens &tokens() const;

    /// Instance-level `md.comp.*` overrides for this badge alone. Mutating
    /// through this accessor drops the cached token set, so an override takes
    /// effect on the next repaint without any further call.
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
    void textChanged(const QString &text);

protected:
    void changeEvent(QEvent *event) override;

private slots:
    void onThemeChanged();

private:
    void init();
    /// Drop the cached token set; the next tokens() call rebuilds it.
    void invalidateTokens();

    QString m_text;

    mutable MdBadgeTokens m_tokens;
    mutable bool m_tokensDirty = true;
    MdComponentTokens m_componentTokens;
};

} // namespace md

#endif // MD_BADGE_H
