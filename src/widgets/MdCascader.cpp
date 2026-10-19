#include "MdCascader.h"

#include "MdTextField.h"
#include "core/MdIcon.h"
#include "core/MdShape.h"
#include "core/MdStateLayer.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "private/MdPopup.h"
#include "styles/MdStyleBase.h"

#include <QtGui/QFontMetrics>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>

namespace md {

namespace {

constexpr qreal kRowHeight = 56.0;
constexpr qreal kPanelWidth = 160.0;
constexpr qreal kRowPad = 16.0;
constexpr qreal kGap = 8.0;

} // namespace

// --- MdCascaderNode ---------------------------------------------------------

MdCascaderNode::MdCascaderNode(const QString &label)
    : m_label(label)
{
}

MdCascaderNode::~MdCascaderNode()
{
    qDeleteAll(m_children);
    m_children.clear();
}

void MdCascaderNode::setLabel(const QString &label)
{
    m_label = label;
}

MdCascaderNode *MdCascaderNode::childAt(int index) const
{
    if (index < 0 || index >= int(m_children.size())) {
        return nullptr;
    }
    return m_children.at(index);
}

void MdCascaderNode::appendChild(MdCascaderNode *child)
{
    if (!child) {
        return;
    }
    child->m_parent = this;
    m_children.append(child);
}

void MdCascaderNode::clearChildren()
{
    qDeleteAll(m_children);
    m_children.clear();
}

// --- panel helper -----------------------------------------------------------

/// One cascade column. Painted on list one-line metrics.
class MdCascaderPanel : public QWidget
{
public:
    explicit MdCascaderPanel(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setFocusPolicy(Qt::NoFocus);
        setAttribute(Qt::WA_TransparentForMouseEvents, true);
    }

    QStringList labels;
    QList<int> parents; // index into the previous column's labels, -1 for root
    int highlight = -1;
    int depth = 0;

    QSize sizeHint() const override
    {
        return QSize(int(kPanelWidth), int(qMax(1.0, qreal(labels.size())) * kRowHeight));
    }

    int rowAt(const QPointF &pos) const
    {
        const int row = int(pos.y() / kRowHeight);
        return (row >= 0 && row < labels.size()) ? row : -1;
    }

    QRectF rowRect(int row) const
    {
        return QRectF(0, row * kRowHeight, width(), kRowHeight);
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event);
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);

        const QRectF bounds(0, 0, width(), height());
        const QPainterPath path = MdShape::roundedRect(bounds, ShapeCorner::ExtraSmall);
        painter.fillPath(path, MdTheme::instance().color(ColorRole::SurfaceContainer));

        for (int i = 0; i < labels.size(); ++i) {
            const QRectF row = rowRect(i);
            if (i == highlight) {
                const QPainterPath rowPath = MdShape::roundedRect(row, ShapeCorner::ExtraSmall);
                painter.fillPath(rowPath,
                                 MdTheme::instance().color(ColorRole::SecondaryContainer));
            }
            painter.setFont(MdTypeScale::font(TypeStyle::BodyLarge));
            painter.setPen(i == highlight
                               ? MdTheme::instance().color(ColorRole::OnSecondaryContainer)
                               : MdTheme::instance().color(ColorRole::OnSurface));
            painter.drawText(row.adjusted(kRowPad, 0, -kRowPad - 16, 0),
                             Qt::AlignLeft | Qt::AlignVCenter | Qt::TextSingleLine, labels.at(i));
        }
    }
};

// --- MdCascader -------------------------------------------------------------

MdCascader::MdCascader(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdCascader::~MdCascader()
{
    // Popup is a top-level Qt::Popup window; tear it down before members die
    // so no slot fires on a half-destroyed MdCascader.
    disconnect(this, nullptr, nullptr, nullptr);
    if (m_popup) {
        m_popup->hide();
        delete m_popup;
        m_popup = nullptr;
    }
    qDeleteAll(m_roots);
    m_roots.clear();
}

void MdCascader::init()
{
    setFocusPolicy(Qt::NoFocus);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setAccessibleName(QStringLiteral("cascader"));

    m_field = new MdTextField(this);
    m_field->setVariant(MdTextFieldVariant::Filled);
    m_field->setReadOnly(true);
    m_field->setObjectName(QStringLiteral("md-cascader-field"));
    // QLineEdit has no clicked(); the field is read-only so a press is an
    // open/close toggle. Install a filter on the field itself.
    m_field->installEventFilter(this);

    MdStyleBase::connectThemeUpdate(this, &MdCascader::onThemeChanged);
}

QString MdCascader::labelText() const
{
    return m_field ? m_field->labelText() : QString();
}

void MdCascader::setLabelText(const QString &label)
{
    if (m_field) {
        m_field->setLabelText(label);
    }
    emit labelTextChanged(label);
}

QString MdCascader::placeholderText() const
{
    return m_field ? m_field->placeholderText() : QString();
}

void MdCascader::setPlaceholderText(const QString &placeholder)
{
    if (m_field) {
        m_field->setPlaceholderText(placeholder);
    }
    emit placeholderTextChanged(placeholder);
}

void MdCascader::addTopLevelNode(MdCascaderNode *node)
{
    if (!node) {
        return;
    }
    node->m_parent = nullptr;
    m_roots.append(node);
}

void MdCascader::clearNodes()
{
    qDeleteAll(m_roots);
    m_roots.clear();
    m_openPath.clear();
}

void MdCascader::setCurrentPath(const QStringList &path)
{
    commitPath(path);
}

void MdCascader::showPopup()
{
    if (m_popupVisible) {
        return;
    }
    rebuildPanels();
    if (!m_popup) {
        // Qt::Popup top-level, like MdSelect's menu. Never setVisible(true)
        // from a gallery bank — a trigger button opens it.
        m_popup = new QWidget(nullptr, Qt::Popup | Qt::FramelessWindowHint);
        m_popup->setAttribute(Qt::WA_DeleteOnClose, false);
        connect(m_popup, &QWidget::destroyed, this, [this]() {
            m_popup = nullptr;
            if (m_popupVisible) {
                m_popupVisible = false;
                emit popupVisibleChanged(false);
            }
        });
    }
    relayout();
    const QPoint origin = mapToGlobal(QPoint(0, height() + 4));
    m_popup->move(origin);
    showPopupWithoutFlash(m_popup);
    m_popupVisible = true;
    emit popupVisibleChanged(true);
}

void MdCascader::hidePopup()
{
    closePopup();
}

void MdCascader::closePopup()
{
    if (m_popup) {
        m_popup->hide();
    }
    if (m_popupVisible) {
        m_popupVisible = false;
        emit popupVisibleChanged(false);
    }
}

void MdCascader::commitPath(const QStringList &path)
{
    if (m_currentPath == path) {
        return;
    }
    m_currentPath = path;
    if (m_field) {
        m_field->setText(path.join(QStringLiteral(" / ")));
    }
    emit currentPathChanged(m_currentPath);
    emit pathSelected(m_currentPath);
    closePopup();
}

void MdCascader::rebuildPanels()
{
    qDeleteAll(m_panels);
    m_panels.clear();
    m_highlight.clear();

    if (!m_popup) {
        return;
    }

    QList<MdCascaderNode *> level = m_roots;
    QList<int> parents;
    parents.append(-1);
    int depth = 0;

    while (!level.isEmpty()) {
        auto *panel = new MdCascaderPanel(m_popup);
        panel->depth = depth;
        for (int i = 0; i < level.size(); ++i) {
            panel->labels.append(level.at(i)->label());
            panel->parents.append(parents.value(i, -1));
        }
        panel->highlight = -1;
        m_panels.append(panel);
        m_highlight.append(-1);

        // Advance along the open path when present.
        QList<MdCascaderNode *> next;
        QList<int> nextParents;
        const QString want = m_openPath.value(depth);
        int chosen = -1;
        for (int i = 0; i < level.size(); ++i) {
            if (!want.isEmpty() && level.at(i)->label() == want) {
                chosen = i;
            }
        }
        if (chosen >= 0) {
            m_highlight[depth] = chosen;
            panel->highlight = chosen;
            MdCascaderNode *node = level.at(chosen);
            for (int c = 0; c < node->children().size(); ++c) {
                next.append(node->childAt(c));
                nextParents.append(chosen);
            }
        }
        level = next;
        parents = nextParents;
        ++depth;
    }

    relayout();
}

void MdCascader::onFieldClicked()
{
    if (m_popupVisible) {
        closePopup();
    } else {
        showPopup();
    }
}

bool MdCascader::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_field && event->type() == QEvent::MouseButtonRelease) {
        onFieldClicked();
        return true;
    }
    return QWidget::eventFilter(watched, event);
}

void MdCascader::onPopupClosed()
{
    m_popupVisible = false;
    emit popupVisibleChanged(false);
}

void MdCascader::relayout()
{
    if (!m_popup) {
        return;
    }
    qreal x = 0;
    qreal h = kRowHeight;
    for (QWidget *panel : m_panels) {
        const qreal ph = panel->sizeHint().height();
        panel->setGeometry(QRectF(x, 0, kPanelWidth, ph).toRect());
        h = qMax(h, ph);
        x += kPanelWidth + kGap;
        panel->show();
        panel->raise();
    }
    m_popup->resize(qRound(qMax(kPanelWidth, x - kGap)), qRound(h));
}

void MdCascader::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (m_field) {
        m_field->setGeometry(rect());
    }
}

void MdCascader::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape) {
        closePopup();
        return;
    }
    if (event->key() == Qt::Key_Down || event->key() == Qt::Key_Return
        || event->key() == Qt::Key_Enter) {
        showPopup();
        return;
    }
    QWidget::keyPressEvent(event);
}

void MdCascader::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LayoutDirectionChange || event->type() == QEvent::FontChange) {
        relayout();
        updateGeometry();
    }
    QWidget::changeEvent(event);
}

void MdCascader::onThemeChanged()
{
    for (QWidget *panel : m_panels) {
        panel->update();
    }
    update();
}

} // namespace md
