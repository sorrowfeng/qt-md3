// Visual probe: captures MdSelect field states and MdPopover open/close frames
// into a directory so before/after evidence can be compared pixel-by-pixel.
//
// Usage: visual-probe <output-dir>

#include "core/MdTheme.h"
#include "core/MdTypes.h"
#include "private/MdPopup.h"
#include "widgets/MdButton.h"
#include "widgets/MdMenu.h"
#include "widgets/MdPopover.h"
#include "widgets/MdSelect.h"
#include "widgets/MdTextField.h"

#include <QtCore/QDir>
#include <QtCore/QEvent>
#include <QtCore/QEventLoop>
#include <QtCore/QTimer>
#include <QtGui/QGuiApplication>
#include <QtGui/QImage>
#include <QtGui/QMouseEvent>
#include <QtGui/QScreen>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>

#include <cstdio>

using namespace md;

namespace {

QString g_dir;

void save(const QImage &img, const QString &name)
{
    const QString path = QDir(g_dir).filePath(name);
    if (!img.save(path)) {
        std::fprintf(stderr, "visual-probe: failed to save %s\n", qPrintable(path));
    } else {
        std::printf("wrote %s\n", qPrintable(path));
    }
}

void save(const QPixmap &pm, const QString &name)
{
    save(pm.toImage(), name);
}

void settle(int ms)
{
    QEventLoop loop;
    QTimer::singleShot(ms, &loop, &QEventLoop::quit);
    loop.exec();
}

QImage grabWidget(QWidget *w)
{
    return w->grab().toImage();
}

// Synthesize a hover enter/leave without a real cursor.
void sendHover(QWidget *w, bool enter)
{
    if (enter) {
        QEvent ev(QEvent::Enter);
        QApplication::sendEvent(w, &ev);
        QHoverEvent he(QEvent::HoverEnter, QPointF(10, 10), QPointF(-1, -1));
        QApplication::sendEvent(w, &he);
    } else {
        QEvent ev(QEvent::Leave);
        QApplication::sendEvent(w, &ev);
        QHoverEvent he(QEvent::HoverLeave, QPointF(-1, -1), QPointF(10, 10));
        QApplication::sendEvent(w, &he);
    }
}

void captureSelectStates(MdSelect *select, const QString &stem)
{
    // Rest (field as laid out inside the select).
    auto *field = select->findChild<MdTextField *>();
    if (!field) {
        std::fprintf(stderr, "visual-probe: no MdTextField inside MdSelect\n");
        return;
    }

    // Drop focus so "rest" is really unfocused (window show focuses the
    // first tab stop).
    if (QWidget *window = select->window()) {
        window->setFocus(Qt::OtherFocusReason);
    }
    settle(30);
    save(grabWidget(select), stem + QLatin1String("-rest.png"));

    sendHover(field, true);
    settle(30);
    save(grabWidget(select), stem + QLatin1String("-hover.png"));
    sendHover(field, false);
    settle(30);

    // Keyboard focus (Tab-like).
    select->setFocus(Qt::TabFocusReason);
    settle(30);
    save(grabWidget(select), stem + QLatin1String("-focus.png"));

    // Open the popup and grab the select + the menu window.
    select->showPopup();
    settle(80);
    save(grabWidget(select), stem + QLatin1String("-open.png"));
    if (QScreen *screen = QGuiApplication::primaryScreen()) {
        const QPixmap shot = screen->grabWindow(0);
        if (!shot.isNull()) {
            // Crop around the select for a tighter evidence frame.
            const QRect geo = select->geometry();
            const QPoint tl = select->mapToGlobal(QPoint(0, 0));
            const QRect region(tl.x() - 8, tl.y() - 8,
                               qMin(geo.width() + 280, shot.width() - tl.x() + 8),
                               qMin(geo.height() + 220, shot.height() - tl.y() + 8));
            save(shot.copy(region), stem + QLatin1String("-open-screen.png"));
        }
    }
    select->hidePopup();
    settle(120);
    save(grabWidget(select), stem + QLatin1String("-closed.png"));
}

void capturePopoverFrames(const QString &stem)
{
    auto *host = new QWidget;
    host->resize(360, 240);
    host->setObjectName(QStringLiteral("popover-host"));
    auto *trigger = new QPushButton(QStringLiteral("Open popover"), host);
    trigger->move(20, 20);
    auto *body = new QWidget;
    body->setMinimumSize(180, 64);
    auto *label = new QLabel(QStringLiteral("Popover content"), body);
    label->move(8, 8);

    auto *pop = new MdPopover(host);
    pop->setTitle(QStringLiteral("Details"));
    pop->setContentWidget(body);
    pop->setShape(ShapeCorner::Small);

    host->show();
    settle(50);

    const QRect anchor(trigger->mapToGlobal(QPoint(0, 0)), trigger->size());

    // --- open flight ---
    pop->popup(anchor);
    // t0 is the first painted frame after the map.
    save(grabWidget(pop), stem + QLatin1String("-open-t0.png"));
    settle(16);
    save(grabWidget(pop), stem + QLatin1String("-open-t16.png"));
    settle(16);
    save(grabWidget(pop), stem + QLatin1String("-open-t32.png"));
    settle(120);
    save(grabWidget(pop), stem + QLatin1String("-open-settled.png"));
    if (QScreen *screen = QGuiApplication::primaryScreen()) {
        const QPixmap shot = screen->grabWindow(0);
        if (!shot.isNull()) {
            const QPoint tl = pop->mapToGlobal(QPoint(0, 0));
            const QRect region(tl.x() - 24, tl.y() - 24, pop->width() + 48, pop->height() + 48);
            save(shot.copy(region), stem + QLatin1String("-open-settled-screen.png"));
        }
    }

    // --- close flight ---
    pop->hide();
    save(grabWidget(pop), stem + QLatin1String("-close-t0.png"));
    settle(16);
    save(grabWidget(pop), stem + QLatin1String("-close-t16.png"));
    settle(16);
    save(grabWidget(pop), stem + QLatin1String("-close-t32.png"));
    settle(200);
    // After settle the widget is hidden; grab the *host* region on the screen
    // to see residue, and also the (hidden) widget for completeness.
    save(grabWidget(pop), stem + QLatin1String("-close-settled-widget.png"));
    if (QScreen *screen = QGuiApplication::primaryScreen()) {
        const QPixmap shot = screen->grabWindow(0);
        if (!shot.isNull()) {
            const QPoint tl = trigger->mapToGlobal(QPoint(0, 0));
            const QRect region(tl.x() - 24, tl.y() - 24, pop->width() + 48, pop->height() + 48);
            save(shot.copy(region), stem + QLatin1String("-close-settled-screen.png"));
        }
    }

    // A second open after close — residue often only shows on the re-open.
    pop->popup(anchor);
    settle(80);
    if (QScreen *screen = QGuiApplication::primaryScreen()) {
        const QPixmap shot = screen->grabWindow(0);
        if (!shot.isNull()) {
            const QPoint tl = pop->mapToGlobal(QPoint(0, 0));
            const QRect region(tl.x() - 24, tl.y() - 24, pop->width() + 48, pop->height() + 48);
            save(shot.copy(region), stem + QLatin1String("-reopen-screen.png"));
        }
    }
    pop->hide();
    settle(200);

    host->close();
    delete host;
}

void captureMenuFrames(const QString &stem)
{
    // Menu control: same Qt::Popup surface, known-good paint path.
    auto *host = new QWidget;
    host->resize(360, 240);
    auto *trigger = new QPushButton(QStringLiteral("Open menu"), host);
    trigger->move(20, 20);
    auto *menu = new MdMenu(host);
    menu->addItem(QStringLiteral("One"));
    menu->addItem(QStringLiteral("Two"));
    menu->addItem(QStringLiteral("Three"));
    host->show();
    settle(50);

    const QRect anchor(trigger->mapToGlobal(QPoint(0, 0)), trigger->size());
    menu->popup(anchor);
    settle(80);
    save(grabWidget(menu), stem + QLatin1String("-open-settled.png"));
    if (QScreen *screen = QGuiApplication::primaryScreen()) {
        const QPixmap shot = screen->grabWindow(0);
        if (!shot.isNull()) {
            const QPoint tl = menu->mapToGlobal(QPoint(0, 0));
            const QRect region(tl.x() - 24, tl.y() - 24, menu->width() + 48, menu->height() + 48);
            save(shot.copy(region), stem + QLatin1String("-open-settled-screen.png"));
        }
    }
    menu->requestClose();
    settle(200);
    if (QScreen *screen = QGuiApplication::primaryScreen()) {
        const QPixmap shot = screen->grabWindow(0);
        if (!shot.isNull()) {
            const QPoint tl = trigger->mapToGlobal(QPoint(0, 0));
            const QRect region(tl.x() - 24, tl.y() - 24, 280, 220);
            save(shot.copy(region), stem + QLatin1String("-close-settled-screen.png"));
        }
    }
    host->close();
    delete host;
}

} // namespace

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    if (argc < 2) {
        std::fprintf(stderr, "usage: visual-probe <output-dir>\n");
        return 2;
    }
    g_dir = QString::fromLocal8Bit(argv[1]);
    if (!QDir().mkpath(g_dir)) {
        std::fprintf(stderr, "visual-probe: cannot create %s\n", qPrintable(g_dir));
        return 1;
    }

    MdTheme::instance().setThemeMode(ThemeMode::Light);

    // Host window so the selects are real top-level children (correct fonts).
    auto *window = new QWidget;
    window->resize(420, 420);
    window->setObjectName(QStringLiteral("probe-window"));

    auto *filled = new MdSelect(window);
    filled->setObjectName(QStringLiteral("select-filled"));
    filled->setLabelText(QStringLiteral("Theme"));
    filled->addItem(QStringLiteral("Light"));
    filled->addItem(QStringLiteral("Dark"));
    filled->addItem(QStringLiteral("System"));
    filled->move(24, 24);
    filled->resize(280, filled->sizeHint().height());

    auto *outlined = new MdSelect(window);
    outlined->setObjectName(QStringLiteral("select-outlined"));
    outlined->setVariant(MdTextFieldVariant::Outlined);
    outlined->setLabelText(QStringLiteral("Streaming quality"));
    outlined->addItem(QStringLiteral("Low"));
    outlined->addItem(QStringLiteral("Standard"));
    outlined->addItem(QStringLiteral("High"));
    outlined->setCurrentIndex(1);
    outlined->move(24, 120);
    outlined->resize(280, outlined->sizeHint().height());

    auto *plain = new MdTextField(window);
    plain->setObjectName(QStringLiteral("field-filled"));
    plain->setLabelText(QStringLiteral("Label"));
    plain->setText(QStringLiteral("Value"));
    plain->move(24, 220);
    plain->resize(280, plain->sizeHint().height());

    auto *plainOutline = new MdTextField(window);
    plainOutline->setObjectName(QStringLiteral("field-outlined"));
    plainOutline->setVariant(MdTextFieldVariant::Outlined);
    plainOutline->setLabelText(QStringLiteral("Label"));
    plainOutline->setText(QStringLiteral("Value"));
    plainOutline->move(24, 300);
    plainOutline->resize(280, plainOutline->sizeHint().height());

    window->show();
    settle(80);

    // Let the label-float springs settle so the floated chrome (cut-out,
    // body-small label) is what the "rest" captures show.
    plain->setText(plain->text());
    plainOutline->setText(plainOutline->text());
    settle(250);

    // Full window overview.
    save(grabWidget(window), QStringLiteral("00-window.png"));

    captureSelectStates(filled, QStringLiteral("10-select-filled"));
    captureSelectStates(outlined, QStringLiteral("11-select-outlined"));

    // Direct text-field states (focus ring / outline contract).
    save(grabWidget(plain), QStringLiteral("20-field-filled-rest.png"));
    sendHover(plain, true);
    settle(30);
    save(grabWidget(plain), QStringLiteral("21-field-filled-hover.png"));
    sendHover(plain, false);
    plain->setFocus(Qt::TabFocusReason);
    settle(30);
    save(grabWidget(plain), QStringLiteral("22-field-filled-focus.png"));

    save(grabWidget(plainOutline), QStringLiteral("23-field-outlined-rest.png"));
    sendHover(plainOutline, true);
    settle(30);
    save(grabWidget(plainOutline), QStringLiteral("24-field-outlined-hover.png"));
    sendHover(plainOutline, false);
    plainOutline->setFocus(Qt::TabFocusReason);
    settle(30);
    save(grabWidget(plainOutline), QStringLiteral("25-field-outlined-focus.png"));

    capturePopoverFrames(QStringLiteral("30-popover"));
    captureMenuFrames(QStringLiteral("40-menu"));

    window->close();
    delete window;

    std::printf("visual-probe: done -> %s\n", qPrintable(g_dir));
    return 0;
}
