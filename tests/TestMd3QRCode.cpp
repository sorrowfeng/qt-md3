// TestMd3QRCode — ISO/IEC 18004 encoder coverage and MdQRCode wiring.
//
// The encoder is checked for structural validity (finders, timing, dark
// module, format information) and for a data-path round trip: the bit stream
// recovered from the matrix must reproduce the byte-mode payload. That is the
// "matrix decodes" gate without pulling in an external decoder.

#include <QtTest/QtTest>

#include "core/MdQrEncoder.h"
#include "widgets/MdQRCode.h"

#include <QtGui/QImage>
#include <QtGui/QPainter>

using md::MdQrEcLevel;
using md::MdQrEncodeResult;
using md::MdQrEncoder;
using md::MdQRCode;

namespace {

bool moduleAt(const MdQrEncodeResult &r, int x, int y)
{
    if (x < 0 || y < 0 || x >= r.dimension || y >= r.dimension) {
        return false;
    }
    return r.modules.at(y * r.dimension + x);
}

bool isFinderAt(const MdQrEncodeResult &r, int ox, int oy)
{
    // 7x7 ring + 3x3 core, light separator outside.
    for (int y = -1; y <= 7; ++y) {
        for (int x = -1; x <= 7; ++x) {
            const int mx = ox + x;
            const int my = oy + y;
            const bool outside = x < 0 || y < 0 || x > 6 || y > 6;
            const bool ring = !outside && (x == 0 || x == 6 || y == 0 || y == 6);
            const bool core = x >= 2 && x <= 4 && y >= 2 && y <= 4;
            const bool expectDark = ring || core;
            const bool dark = moduleAt(r, mx, my);
            if (outside) {
                if (dark) {
                    return false;
                }
            } else if (dark != expectDark) {
                return false;
            }
        }
    }
    return true;
}

// Undo the mask and walk the ISO/IEC 18004 8.1 zigzag to recover the raw
// bit stream that `placeData` wrote.
QVector<quint8> recoverBits(const MdQrEncodeResult &r, int mask, int version)
{
    const int dim = r.dimension;
    // Rebuild the function-module map the same way the encoder reserves it.
    QVector<quint8> isFunction(dim * dim, 0);
    auto mark = [&](int x, int y) {
        if (x >= 0 && y >= 0 && x < dim && y < dim) {
            isFunction[y * dim + x] = 1;
        }
    };
    auto markFinder = [&](int ox, int oy) {
        for (int dy = -1; dy <= 7; ++dy) {
            for (int dx = -1; dx <= 7; ++dx) {
                mark(ox + dx, oy + dy);
            }
        }
    };
    markFinder(0, 0);
    markFinder(dim - 7, 0);
    markFinder(0, dim - 7);
    for (int i = 8; i < dim - 8; ++i) {
        mark(6, i);
        mark(i, 6);
    }
    // Alignment patterns.
    QVector<int> centres;
    switch (version) {
    case 1:
        break;
    case 2:
        centres = { 6, 18 };
        break;
    case 3:
        centres = { 6, 22 };
        break;
    case 4:
        centres = { 6, 26 };
        break;
    case 5:
        centres = { 6, 30 };
        break;
    case 6:
        centres = { 6, 34 };
        break;
    case 7:
        centres = { 6, 22, 38 };
        break;
    case 8:
        centres = { 6, 24, 42 };
        break;
    case 9:
        centres = { 6, 26, 46 };
        break;
    case 10:
        centres = { 6, 28, 50 };
        break;
    default:
        break;
    }
    for (int r = 0; r < centres.size(); ++r) {
        for (int c = 0; c < centres.size(); ++c) {
            const int cx = centres.at(c);
            const int cy = centres.at(r);
            const bool topLeft = cx <= 8 && cy <= 8;
            const bool topRight = cx >= dim - 9 && cy <= 8;
            const bool bottomLeft = cx <= 8 && cy >= dim - 9;
            if (topLeft || topRight || bottomLeft) {
                continue;
            }
            for (int dy = -2; dy <= 2; ++dy) {
                for (int dx = -2; dx <= 2; ++dx) {
                    mark(cx + dx, cy + dy);
                }
            }
        }
    }
    // Format + version + dark module.
    for (int i = 0; i <= 8; ++i) {
        mark(i, 8);
        mark(8, i);
    }
    for (int i = 0; i < 8; ++i) {
        mark(dim - 1 - i, 8);
        mark(8, dim - 1 - i);
    }
    mark(8, dim - 8);
    if (version >= 7) {
        for (int i = 0; i < 6; ++i) {
            for (int j = 0; j < 3; ++j) {
                mark(dim - 11 + j, i);
                mark(i, dim - 11 + j);
            }
        }
    }

    QVector<quint8> bits;
    bool upward = true;
    for (int right = dim - 1; right >= 1; right -= 2) {
        if (right == 6) {
            --right;
        }
        for (int vert = 0; vert < dim; ++vert) {
            const int y = upward ? (dim - 1 - vert) : vert;
            for (int k = 0; k < 2; ++k) {
                const int x = right - k;
                if (isFunction[y * dim + x]) {
                    continue;
                }
                bool dark = moduleAt(r, x, y);
                // Undo the mask.
                const int i = y;
                const int j = x;
                bool invert = false;
                switch (mask) {
                case 0:
                    invert = ((i + j) % 2) == 0;
                    break;
                case 1:
                    invert = (i % 2) == 0;
                    break;
                case 2:
                    invert = (j % 3) == 0;
                    break;
                case 3:
                    invert = ((i + j) % 3) == 0;
                    break;
                case 4:
                    invert = ((i / 2 + j / 3) % 2) == 0;
                    break;
                case 5:
                    invert = ((i * j) % 2 + (i * j) % 3) == 0;
                    break;
                case 6:
                    invert = (((i * j) % 2 + (i * j) % 3) % 2) == 0;
                    break;
                case 7:
                    invert = (((i + j) % 2 + (i * j) % 3) % 2) == 0;
                    break;
                default:
                    break;
                }
                if (invert) {
                    dark = !dark;
                }
                bits.append(dark ? 1 : 0);
            }
        }
        upward = !upward;
    }
    return bits;
}

// Extract the byte-mode payload from a recovered bit stream.
QByteArray decodeBytePayload(const QVector<quint8> &bits, int version, bool *ok)
{
    *ok = false;
    int pos = 0;
    auto take = [&](int n) -> quint32 {
        quint32 v = 0;
        for (int i = 0; i < n && pos < bits.size(); ++i) {
            v = (v << 1) | bits.at(pos);
            ++pos;
        }
        return v;
    };
    const quint32 mode = take(4);
    if (mode != 0x4) {
        return QByteArray();
    }
    const int countBits = version >= 10 ? 16 : 8;
    const quint32 count = take(countBits);
    QByteArray out;
    for (quint32 i = 0; i < count; ++i) {
        out.append(char(take(8) & 0xFFu));
    }
    *ok = true;
    return out;
}

// Find the mask by trying all eight and keeping the one whose bit stream
// decodes back to `expected`.
bool roundTrip(const MdQrEncodeResult &r, const QByteArray &expected)
{
    for (int mask = 0; mask < 8; ++mask) {
        const QVector<quint8> bits = recoverBits(r, mask, r.version);
        bool ok = false;
        const QByteArray payload = decodeBytePayload(bits, r.version, &ok);
        if (ok && payload == expected) {
            return true;
        }
    }
    return false;
}

} // namespace

class TestMd3QRCode : public QObject
{
    Q_OBJECT

private slots:
    // --- capacity table ----------------------------------------------------
    void capacityTable()
    {
        QCOMPARE(MdQrEncoder::dimensionForVersion(1), 21);
        QCOMPARE(MdQrEncoder::dimensionForVersion(10), 57);
        QCOMPARE(MdQrEncoder::dimensionForVersion(0), 0);
        QCOMPARE(MdQrEncoder::dimensionForVersion(11), 0);
        // Published ISO/IEC 18004 Table 7 byte-mode capacities.
        QCOMPARE(MdQrEncoder::byteCapacity(1, MdQrEcLevel::L), 17);
        QCOMPARE(MdQrEncoder::byteCapacity(1, MdQrEcLevel::M), 14);
        QCOMPARE(MdQrEncoder::byteCapacity(2, MdQrEcLevel::L), 32);
        QCOMPARE(MdQrEncoder::byteCapacity(7, MdQrEcLevel::L), 154);
        QCOMPARE(MdQrEncoder::byteCapacity(10, MdQrEcLevel::L), 271);
        QCOMPARE(MdQrEncoder::byteCapacity(10, MdQrEcLevel::M), 213);
        QVERIFY(MdQrEncoder::byteCapacity(11, MdQrEcLevel::M) < 0);
    }

    // --- version selection -------------------------------------------------
    void versionSelection()
    {
        const MdQrEncodeResult small =
            MdQrEncoder::encodeText(QStringLiteral("hi"), MdQrEcLevel::M);
        QVERIFY(small.ok);
        QCOMPARE(small.version, 1);
        QCOMPARE(small.dimension, 21);

        const MdQrEncodeResult exact =
            MdQrEncoder::encode(QByteArray(14, 'a'), MdQrEcLevel::M);
        QVERIFY(exact.ok);
        QCOMPARE(exact.version, 1);

        const MdQrEncodeResult next =
            MdQrEncoder::encode(QByteArray(15, 'a'), MdQrEcLevel::M);
        QVERIFY(next.ok);
        QCOMPARE(next.version, 2);
        QCOMPARE(next.dimension, 25);

        const MdQrEncodeResult tooBig =
            MdQrEncoder::encode(QByteArray(500, 'a'), MdQrEcLevel::M);
        QVERIFY(!tooBig.ok);
        QVERIFY(!tooBig.error.isEmpty());
    }

    // --- structure ---------------------------------------------------------
    void finderAndTiming()
    {
        const MdQrEncodeResult r =
            MdQrEncoder::encodeText(QStringLiteral("https://m3.material.io"),
                                    MdQrEcLevel::M);
        QVERIFY(r.ok);
        QCOMPARE(r.modules.size(), r.dimension * r.dimension);

        QVERIFY2(isFinderAt(r, 0, 0), "top-left finder");
        QVERIFY2(isFinderAt(r, r.dimension - 7, 0), "top-right finder");
        QVERIFY2(isFinderAt(r, 0, r.dimension - 7), "bottom-left finder");

        // Timing patterns: row/col 6 alternate starting dark at module 8.
        for (int i = 8; i < r.dimension - 8; ++i) {
            const bool expect = (i % 2) == 0;
            if (moduleAt(r, 6, i) != expect) {
                QFAIL("vertical timing");
            }
            if (moduleAt(r, i, 6) != expect) {
                QFAIL("horizontal timing");
            }
        }

        // Dark module at (8, dimension - 8). ISO/IEC 18004 7.3.5.
        QVERIFY2(moduleAt(r, 8, r.dimension - 8), "dark module");
    }

    // --- format information ------------------------------------------------
    void formatInfoIsIso()
    {
        // Table C.1: EC M, mask 0 is 0x5412. The encoder picks the best mask,
        // so probe the format string of the produced symbol instead and check
        // it decodes to (M, mask) with a valid BCH remainder.
        const MdQrEncodeResult r =
            MdQrEncoder::encodeText(QStringLiteral("QR"), MdQrEcLevel::M);
        QVERIFY(r.ok);

        // Read the 15-bit copy beside the top-left finder (bit 0 = LSB).
        quint16 fmt = 0;
        for (int i = 0; i <= 5; ++i) {
            if (moduleAt(r, 8, i)) {
                fmt |= quint16(1u << i);
            }
        }
        if (moduleAt(r, 8, 7)) {
            fmt |= quint16(1u << 6);
        }
        if (moduleAt(r, 8, 8)) {
            fmt |= quint16(1u << 7);
        }
        if (moduleAt(r, 7, 8)) {
            fmt |= quint16(1u << 8);
        }
        for (int i = 9; i <= 14; ++i) {
            if (moduleAt(r, 14 - i, 8)) {
                fmt |= quint16(1u << i);
            }
        }

        const quint16 raw = quint16(fmt ^ 0x5412u);
        const int data = (raw >> 10) & 0x1F;
        const int ecBits = (data >> 3) & 0x3;
        const int mask = data & 0x7;
        QCOMPARE(ecBits, 0x0); // M
        QVERIFY(mask >= 0 && mask <= 7);

        // BCH check: re-derive the 15-bit value and compare.
        quint32 rem = quint32(data);
        for (int i = 0; i < 10; ++i) {
            rem = (rem << 1) ^ (((rem >> 9) & 1u) ? 0x537u : 0u);
        }
        const quint16 expect = quint16(((quint32(data) << 10) | (rem & 0x3FFu)) ^ 0x5412u);
        QCOMPARE(fmt, expect);

        // The second copy must match.
        quint16 fmt2 = 0;
        for (int i = 0; i <= 7; ++i) {
            if (moduleAt(r, r.dimension - 1 - i, 8)) {
                fmt2 |= quint16(1u << i);
            }
        }
        for (int i = 8; i <= 14; ++i) {
            if (moduleAt(r, 8, r.dimension - 15 + i)) {
                fmt2 |= quint16(1u << i);
            }
        }
        QCOMPARE(fmt2, fmt);
    }

    // --- data path round trip ---------------------------------------------
    void payloadRoundTrips()
    {
        const QList<QByteArray> payloads{
            QByteArray("A"),
            QByteArray("hello"),
            QByteArray("https://m3.material.io"),
            QByteArray(14, 'x'),
            QByteArray(40, 'y'),
        };
        for (const QByteArray &payload : payloads) {
            for (MdQrEcLevel ec : { MdQrEcLevel::L, MdQrEcLevel::M }) {
                const MdQrEncodeResult r = MdQrEncoder::encode(payload, ec);
                QVERIFY2(r.ok, qPrintable(QStringLiteral("encode failed: %1")
                                              .arg(QString::fromUtf8(payload))));
                QVERIFY2(roundTrip(r, payload),
                         qPrintable(QStringLiteral("round-trip failed: %1")
                                        .arg(QString::fromUtf8(payload))));
            }
        }
    }

    // --- quiet zone / widget wiring ---------------------------------------
    void widgetWiresEncoder()
    {
        MdQRCode qr;
        QVERIFY(qr.hasQuietZone());
        qr.setText(QStringLiteral("https://m3.material.io"));
        QVERIFY(qr.matrixDimension() >= 21);
        // Version 2-M at 22 UTF-8 bytes.
        QCOMPARE(qr.matrixDimension(), 25);

        // Quiet zone paints 4 modules around the matrix.
        const QRectF code = qr.codeRect();
        const qreal side = code.width();
        const qreal expected = (qr.matrixDimension() + 8) * qr.moduleSize();
        QVERIFY(qAbs(side - expected) < 0.5);

        // A scannable symbol has dark finder modules the widget must paint.
        QVERIFY(qr.moduleAt(0, 0));
        QVERIFY(qr.moduleAt(3, 3));
        QVERIFY(!qr.moduleAt(1, 1));

        qr.setText(QStringLiteral("hi"));
        QCOMPARE(qr.matrixDimension(), 21);
        QSignalSpy textSpy(&qr, &MdQRCode::textChanged);
        qr.setText(QStringLiteral("other"));
        QCOMPARE(textSpy.count(), 1);
    }

    void rendersNonBlank()
    {
        MdQRCode qr(QStringLiteral("hello"));
        qr.resize(qr.sizeHint());
        qr.show();
        QVERIFY(QTest::qWaitForWindowExposed(&qr));
        QImage image(qr.size(), QImage::Format_ARGB32_Premultiplied);
        image.fill(qRgba(0, 0, 0, 0));
        QPainter painter(&image);
        qr.render(&painter, QPoint(), QRegion(), QWidget::DrawChildren);
        painter.end();
        int painted = 0;
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                if (image.pixel(x, y) != qRgba(0, 0, 0, 0)) {
                    ++painted;
                }
            }
        }
        QVERIFY2(painted > 0, "MdQRCode painted nothing");
    }

    void versionInfoBlockForV7()
    {
        // V7-L byte capacity is 154 (156 data codewords, 12-bit mode+count
        // overhead). 154 bytes forces version 7 so the 6×3 version block lands.
        const QByteArray payload(154, 'z');
        const MdQrEncodeResult r =
            MdQrEncoder::encode(payload, MdQrEcLevel::L, 7, 10);
        QVERIFY(r.ok);
        QCOMPARE(r.version, 7);
        QCOMPARE(r.dimension, 45);
        // Bottom-left version block occupies rows 45-11 = 34..36, cols 0..5.
        // It is function data, so at least one module differs from all-light.
        bool any = false;
        for (int y = r.dimension - 11; y < r.dimension - 8; ++y) {
            for (int x = 0; x < 6; ++x) {
                if (moduleAt(r, x, y)) {
                    any = true;
                }
            }
        }
        QVERIFY2(any, "version information block missing");
        QVERIFY(roundTrip(r, payload));
    }
};

QTEST_MAIN(TestMd3QRCode)
#include "TestMd3QRCode.moc"
