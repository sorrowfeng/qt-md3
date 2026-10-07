#include "core/MdCore.h"

#include <QtCore/QCommandLineOption>
#include <QtCore/QCommandLineParser>
#include <QtCore/QTimer>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

// qt-md3 example application.
//
// The component gallery grows here one page at a time. This shell exists so the
// library links into a real GUI target from day one, and so CI can smoke-test an
// installed binary. It performs no styling of its own: every visual value will
// come from the theme once the Stage 1 base modules land.

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("qt-md3-example"));
    QApplication::setApplicationVersion(QString::fromLatin1(md::libraryVersion()));
    QApplication::setOrganizationName(QStringLiteral("qt-md3"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("qt-md3 component gallery"));
    parser.addHelpOption();
    parser.addVersionOption();

    // CI smoke hook: quit automatically after N milliseconds.
    QCommandLineOption smokeOption(
        QStringLiteral("smoke-exit-ms"),
        QStringLiteral("Quit after the given number of milliseconds (0 disables)."),
        QStringLiteral("ms"),
        QStringLiteral("0"));
    parser.addOption(smokeOption);
    parser.process(app);

    QMainWindow window;
    window.setWindowTitle(
        QStringLiteral("qt-md3 Example %1").arg(QString::fromLatin1(md::libraryVersion())));

    auto *central = new QWidget(&window);
    auto *layout = new QVBoxLayout(central);
    auto *placeholder = new QLabel(
        QStringLiteral("qt-md3 component gallery\n\n"
                       "The Stage 1 base modules and the first MD3 components\n"
                       "will appear here, one page per component."),
        central);
    placeholder->setAlignment(Qt::AlignCenter);
    layout->addWidget(placeholder);
    window.setCentralWidget(central);

    window.resize(1180, 760);
    window.show();

    const int smokeMs = parser.value(smokeOption).toInt();
    if (smokeMs > 0) {
        QTimer::singleShot(smokeMs, &app, &QCoreApplication::quit);
    }

    return app.exec();
}
