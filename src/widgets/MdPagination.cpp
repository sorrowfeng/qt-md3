#include "MdPagination.h"

#include "MdButton.h"
#include "MdIconButton.h"
#include "core/MdTheme.h"
#include "core/MdTypeScale.h"
#include "styles/MdStyleBase.h"

#include <QtGui/QKeyEvent>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>

namespace md {

namespace {

/// Gap between the pager's composed controls.
constexpr int kGap = 4;
constexpr int kEllipsisWidth = 24;

} // namespace

MdPagination::MdPagination(QWidget *parent)
    : QWidget(parent)
{
    init();
}

MdPagination::~MdPagination() = default;

void MdPagination::init()
{
    setFocusPolicy(Qt::WheelFocus);
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    setAccessibleName(QStringLiteral("pagination"));

    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(kGap);

    // First/last are always installed; setShowFirstLast toggles visibility so
    // the property is live in both directions without a rebuild.
    m_firstButton = new MdIconButton(QStringLiteral("first_page"), this);
    m_firstButton->setVariant(IconButtonVariant::Standard);
    m_firstButton->setButtonSize(ButtonSize::XSmall);
    m_firstButton->setObjectName(QStringLiteral("md-pagination-first"));
    m_firstButton->setToolTip(QStringLiteral("First page"));
    connect(m_firstButton, &MdIconButton::clicked, this, &MdPagination::firstPage);
    layout->addWidget(m_firstButton);
    m_firstButton->setVisible(m_showFirstLast);

    m_prevButton = new MdIconButton(QStringLiteral("chevron_left"), this);
    m_prevButton->setVariant(IconButtonVariant::Standard);
    m_prevButton->setButtonSize(ButtonSize::XSmall);
    m_prevButton->setObjectName(QStringLiteral("md-pagination-prev"));
    m_prevButton->setToolTip(QStringLiteral("Previous page"));
    connect(m_prevButton, &MdIconButton::clicked, this, &MdPagination::previousPage);
    layout->addWidget(m_prevButton);

    m_nextButton = new MdIconButton(QStringLiteral("chevron_right"), this);
    m_nextButton->setVariant(IconButtonVariant::Standard);
    m_nextButton->setButtonSize(ButtonSize::XSmall);
    m_nextButton->setObjectName(QStringLiteral("md-pagination-next"));
    m_nextButton->setToolTip(QStringLiteral("Next page"));
    connect(m_nextButton, &MdIconButton::clicked, this, &MdPagination::nextPage);
    layout->addWidget(m_nextButton);

    m_lastButton = new MdIconButton(QStringLiteral("last_page"), this);
    m_lastButton->setVariant(IconButtonVariant::Standard);
    m_lastButton->setButtonSize(ButtonSize::XSmall);
    m_lastButton->setObjectName(QStringLiteral("md-pagination-last"));
    m_lastButton->setToolTip(QStringLiteral("Last page"));
    connect(m_lastButton, &MdIconButton::clicked, this, &MdPagination::lastPage);
    layout->addWidget(m_lastButton);
    m_lastButton->setVisible(m_showFirstLast);

    // Page buttons and ellipses are inserted between prev and next.
    rebuild();

    MdStyleBase::connectThemeUpdate(this, &MdPagination::onThemeChanged);
}

void MdPagination::setPageCount(int count)
{
    const int clamped = qMax(1, count);
    if (m_pageCount == clamped) {
        return;
    }
    m_pageCount = clamped;
    if (m_currentPage > m_pageCount) {
        setCurrentPage(m_pageCount);
    }
    rebuild();
    emit pageCountChanged(m_pageCount);
}

void MdPagination::setCurrentPage(int page)
{
    const int clamped = qBound(1, page, m_pageCount);
    if (m_currentPage == clamped) {
        return;
    }
    m_currentPage = clamped;
    rebuild();
    emit currentPageChanged(m_currentPage);
    emit pageSelected(m_currentPage);
}

void MdPagination::setVisiblePages(int count)
{
    const int clamped = qBound(1, count, 21);
    if (m_visiblePages == clamped) {
        return;
    }
    m_visiblePages = clamped;
    rebuild();
    emit visiblePagesChanged(m_visiblePages);
}

void MdPagination::setShowFirstLast(bool show)
{
    if (m_showFirstLast == show) {
        return;
    }
    m_showFirstLast = show;
    if (m_firstButton) {
        m_firstButton->setVisible(m_showFirstLast);
    }
    if (m_lastButton) {
        m_lastButton->setVisible(m_showFirstLast);
    }
    applyEnabledStates();
    emit showFirstLastChanged(m_showFirstLast);
}

void MdPagination::setWrap(bool wrap)
{
    if (m_wrap == wrap) {
        return;
    }
    m_wrap = wrap;
    applyEnabledStates();
    emit wrapChanged(m_wrap);
}

void MdPagination::previousPage()
{
    if (m_currentPage > 1) {
        setCurrentPage(m_currentPage - 1);
    } else if (m_wrap && m_pageCount > 1) {
        setCurrentPage(m_pageCount);
    }
}

void MdPagination::nextPage()
{
    if (m_currentPage < m_pageCount) {
        setCurrentPage(m_currentPage + 1);
    } else if (m_wrap && m_pageCount > 1) {
        setCurrentPage(1);
    }
}

void MdPagination::firstPage()
{
    setCurrentPage(1);
}

void MdPagination::lastPage()
{
    setCurrentPage(m_pageCount);
}

void MdPagination::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) {
    case Qt::Key_Left:
        previousPage();
        event->accept();
        return;
    case Qt::Key_Right:
        nextPage();
        event->accept();
        return;
    case Qt::Key_Home:
        firstPage();
        event->accept();
        return;
    case Qt::Key_End:
        lastPage();
        event->accept();
        return;
    default:
        break;
    }
    QWidget::keyPressEvent(event);
}

void MdPagination::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LayoutDirectionChange) {
        // QHBoxLayout mirrors automatically; just refresh the enabled set.
        applyEnabledStates();
    }
    QWidget::changeEvent(event);
}

void MdPagination::onThemeChanged()
{
    update();
}

void MdPagination::onButtonClicked()
{
    auto *button = qobject_cast<QPushButton *>(sender());
    if (button == nullptr) {
        return;
    }
    bool ok = false;
    const int page = button->property("mdPage").toInt(&ok);
    if (ok) {
        setCurrentPage(page);
    }
}

void MdPagination::clearButtons()
{
    for (MdButton *button : m_pageButtons) {
        if (button) {
            button->deleteLater();
        }
    }
    m_pageButtons.clear();
    for (QWidget *label : m_ellipsisLabels) {
        if (label) {
            label->deleteLater();
        }
    }
    m_ellipsisLabels.clear();
}

QList<int> MdPagination::visiblePagePlan() const
{
    // Build the page list around the current page. 0 is an ellipsis gap.
    QList<int> plan;
    const int windowSize = m_visiblePages;
    if (m_pageCount <= windowSize + 2) {
        for (int p = 1; p <= m_pageCount; ++p) {
            plan.append(p);
        }
        return plan;
    }

    const int half = windowSize / 2;
    int start = m_currentPage - half;
    int end = start + windowSize - 1;
    if (start < 2) {
        start = 2;
        end = 1 + windowSize;
    }
    if (end > m_pageCount - 1) {
        end = m_pageCount - 1;
        start = m_pageCount - windowSize;
    }

    plan.append(1);
    if (start > 2) {
        plan.append(0);
    }
    for (int p = start; p <= end; ++p) {
        plan.append(p);
    }
    if (end < m_pageCount - 1) {
        plan.append(0);
    }
    plan.append(m_pageCount);
    return plan;
}

void MdPagination::rebuild()
{
    clearButtons();

    auto *layout = qobject_cast<QHBoxLayout *>(this->layout());
    if (layout == nullptr) {
        return;
    }

    // Insert page widgets before the next button (index depends on first/last).
    int insertAt = layout->count();
    for (int i = 0; i < layout->count(); ++i) {
        if (layout->itemAt(i)->widget() == m_nextButton) {
            insertAt = i;
            break;
        }
    }

    const QList<int> plan = visiblePagePlan();
    for (int page : plan) {
        if (page == 0) {
            auto *label = new QLabel(QStringLiteral("…"), this);
            label->setObjectName(QStringLiteral("md-pagination-gap"));
            label->setFixedWidth(kEllipsisWidth);
            label->setAlignment(Qt::AlignCenter);
            label->setFont(MdTypeScale::font(TypeStyle::LabelLarge));
            QPalette palette = label->palette();
            palette.setColor(QPalette::WindowText,
                             MdTheme::instance().color(ColorRole::OnSurfaceVariant));
            label->setPalette(palette);
            m_ellipsisLabels.append(label);
            layout->insertWidget(insertAt++, label);
            continue;
        }

        auto *button = new MdButton(QString::number(page), this);
        button->setVariant(page == m_currentPage ? ButtonVariant::Filled : ButtonVariant::Text);
        button->setButtonSize(ButtonSize::XSmall);
        button->setObjectName(QStringLiteral("md-pagination-page"));
        button->setProperty("mdPage", page);
        button->setAccessibleName(QStringLiteral("Page %1").arg(page));
        connect(button, &MdButton::clicked, this, &MdPagination::onButtonClicked);
        m_pageButtons.append(button);
        layout->insertWidget(insertAt++, button);
    }

    applyEnabledStates();
}

void MdPagination::applyEnabledStates()
{
    const bool hasPrev = m_currentPage > 1 || m_wrap;
    const bool hasNext = m_currentPage < m_pageCount || m_wrap;
    if (m_prevButton) {
        m_prevButton->setEnabled(hasPrev);
    }
    if (m_nextButton) {
        m_nextButton->setEnabled(hasNext);
    }
    if (m_firstButton) {
        m_firstButton->setEnabled(m_currentPage > 1);
    }
    if (m_lastButton) {
        m_lastButton->setEnabled(m_currentPage < m_pageCount);
    }
    for (MdButton *button : m_pageButtons) {
        if (button) {
            const int page = button->property("mdPage").toInt();
            button->setVariant(page == m_currentPage ? ButtonVariant::Filled
                                                     : ButtonVariant::Text);
        }
    }
}

} // namespace md
