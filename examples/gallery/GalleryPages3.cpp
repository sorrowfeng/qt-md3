// Gallery pages, part 3: interaction feedback (state layers, ripple, focus
// ring) and the icon back ends.

#include "GalleryPages.h"

#include "I18n.h"

#include <QtCore/QVariant>
#include <QtCore/QStringList>
#include <QtGui/QFontMetricsF>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>

#include <cmath>

namespace gallery {

namespace {

constexpr qreal kTargetHeight = 96.0;
constexpr qreal kTargetGap = 16.0;

/// The four state-layer opacities, so the demo can show them side by side.
struct StateDemo
{
    md::StateLayerKind kind;
};

const StateDemo kStateDemos[] = {
    {md::StateLayerKind::Hover},
    {md::StateLayerKind::Focus},
    {md::StateLayerKind::Pressed},
    {md::StateLayerKind::Dragged},
};

} // namespace

// ---------------------------------------------------------------------------
// InteractionPage
// ---------------------------------------------------------------------------

InteractionPage::InteractionPage(QWidget *parent)
    : GalleryPage(parent)
{
    setFocusPolicy(Qt::StrongFocus);

    const struct
    {
        const char *label;
        const char *icon;
    } seeds[] = {
        {"press me", "favorite"},
        {"and me", "star"},
        {"keyboard", "settings"},
        {"or me", "notifications"},
    };
    for (const auto &seed : seeds) {
        Target target;
        target.label = QString::fromUtf8(seed.label);
        target.icon = QString::fromUtf8(seed.icon);
        m_targets.append(target);

        auto *ripple = new md::MdRippleController(this);
        connect(ripple, &md::MdRippleController::repaintRequested, this,
                qOverload<>(&QWidget::update));
        m_ripples.append(ripple);
    }
    m_focusRing = new md::MdFocusRingController(this);
    m_focusRing->setSpec(md::MdFocusRingSpec());
    connect(m_focusRing, &md::MdFocusRingController::repaintRequested, this,
            qOverload<>(&QWidget::update));
}

QString InteractionPage::title() const
{
    return L("交互反馈", "Interaction feedback");
}

QString InteractionPage::slug() const
{
    return QStringLiteral("Interaction feedback");
}

QString InteractionPage::subtitle() const
{
    return L("状态层、按压涟漪与焦点指示器——来按一下。",
             "State layers, the press ripple and the focus indicator — press one.");
}

void InteractionPage::build(GalleryContext &context)
{
    QPainter *painter = context.painter();

    context.section(L("两套独立机制", "Two independent mechanisms"));
    context.paragraph(L(
        "悬停与聚焦会为组件着色，并在状态持续期间保持着色。涟漪则不同：它只在按压的"
        "一瞬间存在，从触点出发以圆形扩散，并被裁剪进组件当前的圆角形状。二者刻意分成"
        "两条代码路径，因为把它们合并正是控件出现永不消失的着色的原因。",
        "Hover and focus tint a component and keep it tinted for as long as the state holds. "
        "A ripple is different: it only exists for the moment of the press, expands as a "
        "circle from the point that was touched, and is clipped to whatever corner shape the "
        "component currently has. They are separate code paths on purpose, because merging "
        "them is how a control ends up with a tint that never goes away."));

    // Lay the targets out across the available width.
    const QStringList targetLabels = {L("点我", "press me"), L("还有我", "and me"),
                                      L("键盘", "keyboard"), L("换我", "or me")};
    const qreal targetWidth =
        qMax<qreal>((context.width() - kTargetGap * (m_targets.size() - 1))
                        / qMax<qreal>(qreal(m_targets.size()), 1.0),
                    120.0);
    const QRectF row = context.band(kTargetHeight + 20.0);
    for (int i = 0; i < m_targets.size(); ++i) {
        m_targets[i].label = targetLabels.value(i);
        m_targets[i].rect = QRectF(row.left() + i * (targetWidth + kTargetGap), row.top(),
                                   targetWidth, kTargetHeight);
        m_ripples[i]->setBounds(m_targets[i].rect.size());
        m_ripples[i]->setContentColor(role(md::ColorRole::OnSurfaceVariant));
    }

    if (painter != nullptr) {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        for (const Target &target : m_targets) {
            fillRounded(*painter, target.rect, role(md::ColorRole::SurfaceContainer),
                        md::ShapeCorner::Large);

            const QRectF iconBox(target.rect.center().x() - 12.0, target.rect.top() + 18.0, 24.0,
                                 24.0);
            md::MdIcon::paint(painter, iconBox, target.icon, role(md::ColorRole::Primary));

            painter->setFont(fontFor(md::TypeStyle::LabelMedium));
            painter->setPen(role(md::ColorRole::OnSurface));
            painter->drawText(QRectF(target.rect.left(), target.rect.bottom() - 34.0,
                                     target.rect.width(), 22.0),
                              Qt::AlignCenter, target.label);
        }
        painter->restore();
    }
    context.space(24.0);

    context.section(L("状态层透明度", "State layer opacities"));
    context.paragraph(L(
        "同一时刻只会应用一层。当多个状态叠加时，MD3 不会把透明度相加——由最强的单一"
        "状态胜出，这就是下方色块呈现离散台阶而非叠加和的原因。",
        "Only one layer is ever applied at a time. When several states overlap MD3 does not "
        "stack the opacities — the strongest single state wins, which is why the chip below "
        "shows a discrete step rather than a sum."));

    const qreal chipWidth = 150.0;
    for (int i = 0; i < 4; ++i) {
        const QRectF chipRow = context.band(56.0);
        if (painter == nullptr) {
            continue;
        }
        const auto kind = kStateDemos[i].kind;
        const QRectF chip(chipRow.left(), chipRow.top() + 6.0, chipWidth, 44.0);
        const QColor base = role(md::ColorRole::SurfaceContainerHigh);
        const QColor tinted = md::MdStateLayer::over(base, role(md::ColorRole::OnSurface),
                                                     kind == md::StateLayerKind::Hover,
                                                     kind == md::StateLayerKind::Focus,
                                                     kind == md::StateLayerKind::Pressed,
                                                     kind == md::StateLayerKind::Dragged);
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        fillRounded(*painter, chip, tinted, md::ShapeCorner::Medium);
        painter->setFont(fontFor(md::TypeStyle::LabelLarge));
        painter->setPen(role(md::ColorRole::OnSurface));
        painter->drawText(chip, Qt::AlignCenter, md::stateLayerKindName(kind));
        painter->restore();

        painter->save();
        painter->setFont(fontFor(md::TypeStyle::BodyMedium));
        painter->setPen(role(md::ColorRole::OnSurfaceVariant));
        painter->drawText(QRectF(chip.right() + 20.0, chipRow.top(), chipRow.width() - chipWidth - 20.0,
                                 chipRow.height()),
                          Qt::AlignLeft | Qt::AlignVCenter,
                          L("透明度 %1", "alpha %1")
                              .arg(md::MdStateLayer::opacity(kind), 0, 'f', 2));
        painter->restore();
    }
    context.space(20.0);

    context.section(L("焦点指示器", "Focus indicator"));
    context.paragraph(L(
        "焦点环厚度为三个设备无关像素，与组件保持两个像素的净距，因此永远不会压到它所指"
        "的对象。获得焦点时它先生长到 8px，再在 600 ms emphasized 曲线的剩余时间内回落，"
        "这正是它醒目的原因。按 Tab，然后用方向键操作。",
        "The ring is three device-independent pixels thick and sits two pixels clear of the "
        "component, so it never overlaps what it is pointing at. On focus it first grows to "
        "8px and then settles back over the rest of a 600 ms emphasized curve, which is what "
        "makes it catch the eye. Press Tab, then use the arrow keys."));
    context.detail(L("宽度 %1 dp · 间距 %2 dp · 生长至 %3 dp",
                     "width %1 dp · gap %2 dp · grow to %3 dp")
                       .arg(md::MdFocusRingSpec().width, 0, 'f', 0)
                       .arg(md::MdFocusRingSpec().outwardOffset, 0, 'f', 0)
                       .arg(md::MdFocusRingSpec().activeWidth, 0, 'f', 0));
    context.detail(L("时长 %1 ms，其中 %2 ms 生长 + %3 ms 回落",
                     "duration %1 ms, split %2 ms grow + %3 ms settle")
                       .arg(md::MdFocusRing::totalMs())
                       .arg(md::MdFocusRing::growMs())
                       .arg(md::MdFocusRing::settleMs()));
}

QRectF InteractionPage::targetAt(int index) const
{
    if (index < 0 || index >= m_targets.size()) {
        return QRectF();
    }
    return m_targets.at(index).rect;
}

int InteractionPage::targetAtPosition(const QPointF &position) const
{
    const QPointF local = position - contentRect().topLeft();
    for (int i = 0; i < m_targets.size(); ++i) {
        if (m_targets.at(i).rect.contains(local)) {
            return i;
        }
    }
    return -1;
}

void InteractionPage::paintEvent(QPaintEvent *event)
{
    GalleryPage::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    beginOverlay(painter);

    for (int i = 0; i < m_targets.size(); ++i) {
        const Target &target = m_targets.at(i);
        if (target.rect.isEmpty()) {
            continue;
        }

        // 1. State layer, composited over the card.
        const QColor base = role(md::ColorRole::SurfaceContainer);
        const QColor tinted =
            md::MdStateLayer::over(base, role(md::ColorRole::OnSurfaceVariant),
                                   i == m_hoveredIndex, i == m_focusedIndex && m_showFocus,
                                   i == m_pressedIndex, false);
        if (tinted != base) {
            fillRounded(painter, target.rect, tinted, md::ShapeCorner::Large);
        }

        // 2. Ripple, clipped to the same shape.
        const QPainterPath clip = md::MdShape::roundedRect(
            target.rect,
            md::MdShape::resolvedRadii(md::ShapeCorner::Large, target.rect.size()));
        m_ripples[i]->setClipPath(clip);
        md::MdRipple::paint(&painter, m_ripples[i]->currentFrame(), clip,
                            role(md::ColorRole::OnSurfaceVariant));

        // 3. Focus ring, drawn last so it sits on top of everything.
        if (i == m_focusedIndex && m_showFocus) {
            md::MdFocusRing::paint(
                &painter, target.rect, md::MdShape::resolvedRadii(md::ShapeCorner::Large,
                                                                  target.rect.size()),
                role(md::ColorRole::Secondary), m_focusRing->spec(),
                m_focusRing->isAnimating() ? m_focusRing->elapsedMs() : -1);
        }
    }
    painter.end();
}

void InteractionPage::mousePressEvent(QMouseEvent *event)
{
    setFocus(Qt::MouseFocusReason);
    const int index = targetAtPosition(event->pos());
    if (index < 0 || event->button() != Qt::LeftButton) {
        return;
    }
    m_pressedIndex = index;
    m_focusedIndex = index;
    m_showFocus = true;
    m_focusRing->start();
    m_ripples[index]->press(event->pos() - contentRect().topLeft());
    update();
}

void InteractionPage::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    if (m_pressedIndex < 0) {
        return;
    }
    m_ripples[m_pressedIndex]->release();
    m_pressedIndex = -1;
    update();
}

void InteractionPage::mouseMoveEvent(QMouseEvent *event)
{
    const int index = targetAtPosition(event->pos());
    if (index != m_hoveredIndex) {
        m_hoveredIndex = index;
        update();
    }
}

void InteractionPage::keyPressEvent(QKeyEvent *event)
{
    const int count = m_targets.size();
    if (count == 0) {
        return;
    }
    switch (event->key()) {
    case Qt::Key_Left:
        m_focusedIndex = (m_focusedIndex - 1 + count) % count;
        m_showFocus = true;
        m_focusRing->start();
        update();
        return;
    case Qt::Key_Right:
        m_focusedIndex = (m_focusedIndex + 1) % count;
        m_showFocus = true;
        m_focusRing->start();
        update();
        return;
    case Qt::Key_Space:
    case Qt::Key_Return:
    case Qt::Key_Enter:
        m_showFocus = true;
        // Keyboard activation starts the ripple from the centre, which is
        // what material-web does when there is no pointer position.
        m_ripples[m_focusedIndex]->pressCentered();
        m_pressedIndex = m_focusedIndex;
        update();
        return;
    default:
        break;
    }
    GalleryPage::keyPressEvent(event);
}

void InteractionPage::focusInEvent(QFocusEvent *event)
{
    Q_UNUSED(event);
    m_showFocus = true;
    m_focusRing->start();
    update();
}

void InteractionPage::focusOutEvent(QFocusEvent *event)
{
    Q_UNUSED(event);
    if (m_pressedIndex >= 0) {
        m_ripples[m_pressedIndex]->release();
        m_pressedIndex = -1;
    }
    m_focusRing->stop();
    m_showFocus = false;
    update();
}

// ---------------------------------------------------------------------------
// IconPage
// ---------------------------------------------------------------------------

QString IconPage::title() const
{
    return L("图标", "Icons");
}

QString IconPage::slug() const
{
    return QStringLiteral("Icons");
}

QString IconPage::subtitle() const
{
    return L("Material Symbols 的四个可变轴，及其下的经典 SVG 内置基线。",
             "Material Symbols' four axes, over the bundled classic SVG baseline.");
}

void IconPage::build(GalleryContext &context)
{
    QPainter *painter = context.painter();
    m_axisRows.clear();

    const md::MdIconSet resolved = md::MdIcon::resolveSet(md::MdIconSet::Auto);

    context.section(L("后端", "Back ends"));
    context.paragraph(L(
        "Material Symbols 是一款带四个可变轴的字体：FILL 把描边字形换成实心版本，wght "
        "改变笔画粗细，GRAD 在不改变步进宽度的情况下调整光学字重，opsz 让字形适应自身"
        "尺寸。这正是它是一套字体而非图标集的原因。",
        "Material Symbols is a variable font with four axes: FILL swaps the outlined glyph for "
        "its filled counterpart, wght changes stroke weight, GRAD changes optical weight "
        "without altering advance width, and opsz adapts the glyph to its size. That is what "
        "makes it a font rather than an icon set."));
    context.detail(L("已知码点             %1", "codepoints known     %1").arg(md::MdIcon::count()));
    context.detail(L("内置经典 SVG         %1", "classic svg bundled  %1")
                       .arg(md::MdIcon::classicNames().size()));
    context.detail(L("解析的后端           %1", "resolved back end    %1")
                       .arg(md::iconSetName(resolved)));
    context.detail(L("Symbols 字体         %1", "symbols font         %1")
                       .arg(md::MdIcon::isFontAvailable(md::MdIconFamily::Outlined)
                                ? L("可用", "available")
                                : L("未安装 —— 回退到 SVG", "not installed — falling back to SVG")));
    context.space(14.0);

    // --- axis chips -------------------------------------------------------
    context.section(L("可变轴", "Axes"));
    context.paragraph(L(
        "点击一个数值即可应用。当 Symbols 字体未安装时，这些色块仍会记录轴值，但下方的 "
        "SVG 基线只有一种字重——这恰恰说明了静态图标集为何满足不了 MD3。",
        "Click a value to apply it. When the Symbols font is not installed the chips still "
        "record the axis values, but the SVG baseline below has only one weight — which is "
        "exactly the limitation that makes a static icon set insufficient for MD3."));

    struct Axis
    {
        const char *name;
        QVector<QVariant> values;
        qreal current;
    };
    Axis axes[] = {
        {"FILL", {QVariant(0.0), QVariant(1.0)}, m_style.fill},
        {"wght", {QVariant(300.0), QVariant(400.0), QVariant(500.0), QVariant(700.0)}, m_style.weight},
        {"GRAD", {QVariant(-25.0), QVariant(0.0), QVariant(50.0), QVariant(200.0)}, m_style.grade},
        {"opsz", {QVariant(20.0), QVariant(24.0), QVariant(40.0), QVariant(48.0)}, m_style.opticalSize},
    };

    for (Axis &axis : axes) {
        const QRectF row = context.band(44.0);
        QVector<Chip> chips;
        qreal x = 0.0;
        QVector<QRectF> rects;
        for (const QVariant &value : axis.values) {
            const QString text = QString::number(value.toDouble(), 'g', 4);
            const QFont chipFont = fontFor(md::TypeStyle::LabelLarge);
            const QFontMetricsF metrics(chipFont);
            const qreal width = metrics.horizontalAdvance(text) + 32.0;
            rects.append(QRectF(x, row.top() + 6.0, width, row.height() - 12.0));
            x += width + 8.0;
        }
        // The label column sits after the chips so the chips start flush left.
        if (painter != nullptr) {
            painter->save();
            painter->setRenderHint(QPainter::Antialiasing, true);
            for (int i = 0; i < axis.values.size(); ++i) {
                const QRectF chip = rects.at(i);
                const bool selected = qFuzzyCompare(axis.current + 1.0, axis.values.at(i).toDouble() + 1.0);
                fillRounded(*painter, chip,
                            selected ? role(md::ColorRole::Primary)
                                     : role(md::ColorRole::SurfaceContainerHighest),
                            md::ShapeCorner::Small);
                painter->setFont(fontFor(md::TypeStyle::LabelLarge));
                painter->setPen(selected ? role(md::ColorRole::OnPrimary)
                                         : role(md::ColorRole::OnSurfaceVariant));
                painter->drawText(chip, Qt::AlignCenter,
                                  QString::number(axis.values.at(i).toDouble(), 'g', 4));
                Chip entry;
                entry.rect = chip;
                entry.value = axis.values.at(i);
                chips.append(entry);
            }
            painter->setFont(fontFor(md::TypeStyle::LabelSmall));
            painter->setPen(role(md::ColorRole::OnSurface));
            painter->drawText(QRectF(x + 16.0, row.top(), 160.0, row.height()),
                              Qt::AlignLeft | Qt::AlignVCenter, QString::fromUtf8(axis.name));
            painter->restore();
        }
        m_axisRows.append(chips);
    }
    context.space(16.0);

    // --- Symbols preview --------------------------------------------------
    context.section(L("Material Symbols 预览", "Material Symbols preview"));
    static const char *kSymbolNames[] = {"home", "favorite", "settings", "search",
                                         "notifications", "palette"};
    const qreal symbolBox = 56.0;
    const QRectF symbolRow = context.band(symbolBox + 26.0);
    if (painter != nullptr) {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        for (int i = 0; i < 6; ++i) {
            const QRectF box(symbolRow.left() + i * (symbolBox + 12.0), symbolRow.top(), symbolBox,
                             symbolBox);
            const QString name = QString::fromUtf8(kSymbolNames[i]);
            const bool drawn = md::MdIcon::paint(painter, box, name,
                                                role(md::ColorRole::OnSurface),
                                                md::MdIconSet::MaterialSymbols, md::MdIconFamily::Outlined,
                                                m_style);
            if (!drawn) {
                painter->setFont(fontFor(md::TypeStyle::LabelSmall));
                painter->setPen(role(md::ColorRole::OnSurfaceVariant));
                painter->drawText(box, Qt::AlignCenter, QStringLiteral("—"));
            }
            painter->setFont(fontFor(md::TypeStyle::LabelSmall));
            painter->setPen(role(md::ColorRole::OnSurfaceVariant));
            painter->drawText(QRectF(box.left(), box.bottom() + 2.0, box.width(), 22.0),
                              Qt::AlignCenter, name);
        }
        painter->restore();
    }
    context.space(20.0);

    // --- classic grid -----------------------------------------------------
    context.section(L("经典 Material Icons 基线", "Classic Material Icons baseline"));
    context.paragraph(L(
        "这是 Material Symbols 出现之前的那套 SVG 图标集。之所以内置它，是为了让库在完全"
        "不安装字体的情况下也能用，但它只是回退方案：单一字重、无光学尺寸、无 grade，"
        "只有当初提交进仓库的那些图标。需要完整图标集的场景应安装可变字体。",
        "The SVG set that shipped before Material Symbols. It is bundled so the library is "
        "useful with no font installation at all, but it is a fallback: one weight, no optical "
        "size, no grade, and only the icons that were checked in. Anything requiring the full "
        "set should install the variable font."));

    const QStringList names = md::MdIcon::classicNames();
    const qreal cell = 72.0;
    const int columns = qMax(1, int(context.width() / cell));
    const int rows = (names.size() + columns - 1) / columns;
    const QRectF grid = context.band(rows * cell);
    if (painter != nullptr) {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        for (int i = 0; i < names.size(); ++i) {
            const int column = i % columns;
            const int row = i / columns;
            const QRectF box(grid.left() + column * cell, grid.top() + row * cell, cell, cell);
            md::MdIcon::paint(painter, QRectF(box.left() + 22.0, box.top() + 8.0, 28.0, 28.0),
                              names.at(i), role(md::ColorRole::OnSurfaceVariant),
                              md::MdIconSet::Classic);
            painter->setFont(fontFor(md::TypeStyle::LabelSmall));
            painter->setPen(role(md::ColorRole::OnSurfaceVariant));
            const QString label = names.at(i);
            painter->drawText(QRectF(box.left(), box.top() + 40.0, cell, 30.0),
                              Qt::AlignHCenter | Qt::AlignTop | Qt::TextWordWrap, label);
        }
        painter->restore();
    }
    context.space(12.0);
    context.chip(L("内置 %1 个图标", "%1 icons bundled").arg(names.size()),
                 role(md::ColorRole::SecondaryContainer),
                 role(md::ColorRole::OnSecondaryContainer));
}

void IconPage::paintEvent(QPaintEvent *event)
{
    GalleryPage::paintEvent(event);
}

void IconPage::mousePressEvent(QMouseEvent *event)
{
    const QPointF local = event->pos() - contentRect().topLeft();
    for (int rowIndex = 0; rowIndex < m_axisRows.size(); ++rowIndex) {
        for (const Chip &chip : m_axisRows.at(rowIndex)) {
            if (!chip.rect.contains(local)) {
                continue;
            }
            const qreal value = chip.value.toDouble();
            switch (rowIndex) {
            case 0: m_style.fill = value; break;
            case 1: m_style.weight = value; break;
            case 2: m_style.grade = value; break;
            case 3: m_style.opticalSize = value; break;
            default: break;
            }
            update();
            return;
        }
    }
}

} // namespace gallery
