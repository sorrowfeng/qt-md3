#include "core/MdCore.h"

#include <QtTest/QtTest>

// Bootstrap smoke test: the library builds, links, and reports a consistent
// semantic version. Token, theme, and render tests arrive with the Stage 1
// base modules.

class TestMd3Version : public QObject
{
    Q_OBJECT

private slots:
    void versionStringIsSemantic()
    {
        const QString version = QString::fromLatin1(md::libraryVersion());
        QVERIFY2(!version.isEmpty(), "libraryVersion() must not be empty");

        const QRegularExpression re(QStringLiteral("^\\d+\\.\\d+\\.\\d+$"));
        QVERIFY2(re.match(version).hasMatch(),
                 qPrintable(QStringLiteral("version '%1' is not MAJOR.MINOR.PATCH").arg(version)));
    }

    void versionComponentsMatchString()
    {
        const QString expected = QStringLiteral("%1.%2.%3")
                                     .arg(md::libraryVersionMajor())
                                     .arg(md::libraryVersionMinor())
                                     .arg(md::libraryVersionPatch());
        QCOMPARE(QString::fromLatin1(md::libraryVersion()), expected);
    }
};

QTEST_GUILESS_MAIN(TestMd3Version)

#include "TestMd3Version.moc"
