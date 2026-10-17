#include "MdQrEncoder.h"

// ISO/IEC 18004:2015 (QR Code) byte-mode encoder, versions 1-10, EC L/M.
//
// Table citations in this file refer to ISO/IEC 18004:2015:
//   * Table 1 / 7.4.1  — mode indicators (byte = 0b0100).
//   * Table 3          — character count indicator bit widths.
//   * Table 7 / Annex E — total codewords and EC block structure.
//   * Table 9          — alignment pattern positions.
//   * 8.1 / Figure 14  — data placement (two-module-wide zigzag).
//   * 8.8 / Table 24   — data mask patterns and their predicates.
//   * 8.8.2 / Table 25 — mask evaluation penalty rules N1-N4.
//   * 8.9 / Annex A    — format information BCH(15,5).
//   * 8.10 / Annex D   — version information BCH(18,6), versions 7-10.
//   * Annex A / 8.5    — Reed-Solomon generator polynomials over GF(256),
//                        primitive polynomial 0x11D, primitive root 2.

#include <QtCore/QVector>
#include <algorithm>
#include <cmath>

namespace md {

namespace {

// --- Version geometry -------------------------------------------------------

constexpr int kMinVersion = 1;
constexpr int kMaxVersion = 10;

int dimensionOf(int version)
{
    return 17 + 4 * version;
}

// Total codewords (data + EC) per version. ISO/IEC 18004 Table 7 / Annex E.
int totalCodewords(int version)
{
    static const int kTotal[] = {
        0, 26, 44, 70, 100, 134, 172, 196, 242, 292, 346
    };
    if (version < kMinVersion || version > kMaxVersion) {
        return 0;
    }
    return kTotal[version];
}

// Data codewords per version and EC level. Derived from Annex E block
// structure: sum of data codewords across every block.
int dataCodewords(int version, MdQrEcLevel ec)
{
    // version 1..10 × {L, M}
    static const int kData[2][11] = {
        { 0, 19, 34, 55, 80, 108, 136, 156, 194, 232, 274 }, // L
        { 0, 16, 28, 44, 64, 86, 108, 124, 154, 182, 216 }  // M
    };
    if (version < kMinVersion || version > kMaxVersion) {
        return 0;
    }
    return kData[int(ec)][version];
}

// EC codewords per block. ISO/IEC 18004 Annex E.
int ecCodewordsPerBlock(int version, MdQrEcLevel ec)
{
    static const int kEc[2][11] = {
        { 0, 7, 10, 15, 20, 26, 18, 20, 24, 30, 18 },  // L
        { 0, 10, 16, 26, 18, 24, 16, 18, 22, 22, 26 }  // M
    };
    if (version < kMinVersion || version > kMaxVersion) {
        return 0;
    }
    return kEc[int(ec)][version];
}

// Block structure: (group1Blocks, group1DataCw, group2Blocks, group2DataCw).
struct BlockPlan {
    int g1Blocks = 0;
    int g1Data = 0;
    int g2Blocks = 0;
    int g2Data = 0;
    int ecPerBlock = 0;
};

BlockPlan blockPlan(int version, MdQrEcLevel ec)
{
    BlockPlan plan;
    plan.ecPerBlock = ecCodewordsPerBlock(version, ec);
    // ISO/IEC 18004 Annex E, EC codewords and block counts for 1-10 L/M.
    struct Row {
        int version;
        MdQrEcLevel ec;
        int g1Blocks, g1Data, g2Blocks, g2Data;
    };
    static const Row kRows[] = {
        { 1, MdQrEcLevel::L, 1, 19, 0, 0 },
        { 1, MdQrEcLevel::M, 1, 16, 0, 0 },
        { 2, MdQrEcLevel::L, 1, 34, 0, 0 },
        { 2, MdQrEcLevel::M, 1, 28, 0, 0 },
        { 3, MdQrEcLevel::L, 1, 55, 0, 0 },
        { 3, MdQrEcLevel::M, 1, 44, 0, 0 },
        { 4, MdQrEcLevel::L, 1, 80, 0, 0 },
        { 4, MdQrEcLevel::M, 2, 32, 0, 0 },
        { 5, MdQrEcLevel::L, 1, 108, 0, 0 },
        { 5, MdQrEcLevel::M, 2, 43, 0, 0 },
        { 6, MdQrEcLevel::L, 2, 68, 0, 0 },
        { 6, MdQrEcLevel::M, 4, 27, 0, 0 },
        { 7, MdQrEcLevel::L, 2, 78, 0, 0 },
        { 7, MdQrEcLevel::M, 4, 31, 0, 0 },
        { 8, MdQrEcLevel::L, 2, 97, 0, 0 },
        { 8, MdQrEcLevel::M, 2, 38, 2, 39 },
        { 9, MdQrEcLevel::L, 2, 116, 0, 0 },
        { 9, MdQrEcLevel::M, 3, 36, 2, 37 },
        { 10, MdQrEcLevel::L, 2, 68, 2, 69 },
        { 10, MdQrEcLevel::M, 4, 43, 1, 44 },
    };
    for (const Row &row : kRows) {
        if (row.version == version && row.ec == ec) {
            plan.g1Blocks = row.g1Blocks;
            plan.g1Data = row.g1Data;
            plan.g2Blocks = row.g2Blocks;
            plan.g2Data = row.g2Data;
            return plan;
        }
    }
    return BlockPlan();
}

// Alignment pattern centre coordinates. ISO/IEC 18004 Table 9.
QList<int> alignmentPositions(int version)
{
    switch (version) {
    case 1:
        return {};
    case 2:
        return { 6, 18 };
    case 3:
        return { 6, 22 };
    case 4:
        return { 6, 26 };
    case 5:
        return { 6, 30 };
    case 6:
        return { 6, 34 };
    case 7:
        return { 6, 22, 38 };
    case 8:
        return { 6, 24, 42 };
    case 9:
        return { 6, 26, 46 };
    case 10:
        return { 6, 28, 50 };
    default:
        return {};
    }
}

// Character-count indicator width for byte mode. ISO/IEC 18004 Table 3.
int countBitsFor(int version)
{
    return version >= 10 ? 16 : 8;
}

// --- GF(256) Reed-Solomon ---------------------------------------------------
//
// ISO/IEC 18004 8.5 / Annex A: generator polynomial of degree n over
// GF(256) with primitive polynomial 0x11D and primitive element 2.

struct Gf256 {
    QVector<quint8> exp; // 512 entries so α^(k) is a direct index
    QVector<quint8> log; // 256 entries, log[0] unused

    Gf256()
    {
        exp.resize(512);
        log.resize(256);
        quint8 x = 1;
        for (int i = 0; i < 255; ++i) {
            exp[i] = x;
            log[x] = quint8(i);
            // multiply by α = 2 modulo 0x11D
            x = quint8(x << 1);
            if (x & 0x80) {
                x = quint8(x ^ 0x1D);
            }
        }
        for (int i = 255; i < 512; ++i) {
            exp[i] = exp[i - 255];
        }
        log[0] = 0;
    }

    quint8 mul(quint8 a, quint8 b) const
    {
        if (a == 0 || b == 0) {
            return 0;
        }
        return exp[int(log[a]) + int(log[b])];
    }
};

const Gf256 &gf()
{
    static const Gf256 kInstance;
    return kInstance;
}

// Monic generator polynomial coefficients, highest degree first. Degree n.
// g(x) = Π_{i=0}^{n-1} (x - α^i).
QVector<quint8> generatorPoly(int n)
{
    QVector<quint8> g;
    g.append(1);
    const Gf256 &f = gf();
    for (int i = 0; i < n; ++i) {
        // Multiply g by (x - α^i) = (x + α^i) in characteristic 2.
        QVector<quint8> next(g.size() + 1, 0);
        const quint8 alpha = f.exp[i];
        for (int j = 0; j < g.size(); ++j) {
            next[j] ^= g[j]; // × x
            next[j + 1] ^= f.mul(g[j], alpha);
        }
        g = next;
    }
    return g;
}

// Reed-Solomon remainder of `data` under g(x) of degree `ecLen`.
// Returns `ecLen` bytes (the EC codewords for one block).
QVector<quint8> rsRemainder(const QVector<quint8> &data, int ecLen)
{
    const Gf256 &f = gf();
    const QVector<quint8> gen = generatorPoly(ecLen);
    QVector<quint8> rem(data.size() + ecLen, 0);
    for (int i = 0; i < data.size(); ++i) {
        rem[i] = data[i];
    }
    for (int i = 0; i < data.size(); ++i) {
        const quint8 factor = rem[i];
        if (factor == 0) {
            continue;
        }
        for (int j = 0; j < gen.size(); ++j) {
            rem[i + j] ^= f.mul(gen[j], factor);
        }
    }
    QVector<quint8> out(ecLen, 0);
    for (int i = 0; i < ecLen; ++i) {
        out[i] = rem[data.size() + i];
    }
    return out;
}

// --- Bit buffer -------------------------------------------------------------

class BitBuffer
{
public:
    void append(quint32 value, int bits)
    {
        for (int i = bits - 1; i >= 0; --i) {
            m_bits.append((value >> i) & 1u);
        }
    }

    int size() const { return m_bits.size(); }

    const QVector<quint8> &bits() const { return m_bits; }

    QVector<quint8> toBytes() const
    {
        QVector<quint8> out;
        out.reserve((m_bits.size() + 7) / 8);
        quint8 current = 0;
        int filled = 0;
        for (quint8 bit : m_bits) {
            current = quint8((current << 1) | (bit & 1u));
            ++filled;
            if (filled == 8) {
                out.append(current);
                current = 0;
                filled = 0;
            }
        }
        if (filled > 0) {
            current = quint8(current << (8 - filled));
            out.append(current);
        }
        return out;
    }

private:
    QVector<quint8> m_bits;
};

// --- Function-pattern matrix ----------------------------------------------

// isFunction[row][col] marks every module reserved for a function pattern
// (including format/version information). Data bits skip these.
struct Matrix {
    int dimension = 0;
    QVector<quint8> modules; // 0 light, 1 dark
    QVector<quint8> isFunction;

    void init(int dim)
    {
        dimension = dim;
        modules.fill(0, dim * dim);
        isFunction.fill(0, dim * dim);
    }

    int index(int x, int y) const { return y * dimension + x; }

    void setFunction(int x, int y, bool dark)
    {
        if (x < 0 || y < 0 || x >= dimension || y >= dimension) {
            return;
        }
        modules[index(x, y)] = dark ? 1 : 0;
        isFunction[index(x, y)] = 1;
    }

    void setModule(int x, int y, bool dark)
    {
        if (x < 0 || y < 0 || x >= dimension || y >= dimension) {
            return;
        }
        modules[index(x, y)] = dark ? 1 : 0;
    }

    bool module(int x, int y) const
    {
        return modules[index(x, y)] != 0;
    }

    bool function(int x, int y) const
    {
        return isFunction[index(x, y)] != 0;
    }
};

void drawFinder(Matrix &m, int ox, int oy)
{
    // 7×7 finder: outer ring, one-module light separator, 3×3 core.
    // ISO/IEC 18004 7.3.1 / Figure 12.
    for (int dy = -1; dy <= 7; ++dy) {
        for (int dx = -1; dx <= 7; ++dx) {
            const int x = ox + dx;
            const int y = oy + dy;
            if (x < 0 || y < 0 || x >= m.dimension || y >= m.dimension) {
                continue;
            }
            const bool inRing = dx >= 0 && dx <= 6 && dy >= 0 && dy <= 6;
            const bool ring = inRing && (dx == 0 || dx == 6 || dy == 0 || dy == 6);
            const bool core = dx >= 2 && dx <= 4 && dy >= 2 && dy <= 4;
            m.setFunction(x, y, ring || core);
        }
    }
}

void drawAlignment(Matrix &m, int cx, int cy)
{
    // 5×5 alignment: outer ring + centre. ISO/IEC 18004 7.3.4.
    for (int dy = -2; dy <= 2; ++dy) {
        for (int dx = -2; dx <= 2; ++dx) {
            const bool ring = qAbs(dx) == 2 || qAbs(dy) == 2;
            const bool centre = dx == 0 && dy == 0;
            m.setFunction(cx + dx, cy + dy, ring || centre);
        }
    }
}

void drawTiming(Matrix &m)
{
    // Row and column 6 alternate from the finder separators inward.
    // ISO/IEC 18004 7.3.3.
    for (int i = 8; i < m.dimension - 8; ++i) {
        const bool dark = (i % 2) == 0;
        m.setFunction(6, i, dark);
        m.setFunction(i, 6, dark);
    }
}

void reserveFormatAreas(Matrix &m)
{
    // Format information lives beside the finders. ISO/IEC 18004 8.9.
    // Horizontal strip under the top-left finder, vertical strip beside it.
    for (int i = 0; i <= 8; ++i) {
        if (!m.function(i, 8)) {
            m.setFunction(i, 8, false);
        }
        if (!m.function(8, i)) {
            m.setFunction(8, i, false);
        }
    }
    for (int i = 0; i < 8; ++i) {
        if (!m.function(m.dimension - 1 - i, 8)) {
            m.setFunction(m.dimension - 1 - i, 8, false);
        }
        if (!m.function(8, m.dimension - 1 - i)) {
            m.setFunction(8, m.dimension - 1 - i, false);
        }
    }
    // Dark module: (8, 4·version + 9) = (8, dimension - 8). ISO/IEC 18004 7.3.5.
    m.setFunction(8, m.dimension - 8, true);
}

void reserveVersionAreas(Matrix &m, int version)
{
    if (version < 7) {
        return;
    }
    // 6×3 version-info blocks near the top-right and bottom-left finders.
    // ISO/IEC 18004 8.10 / Figure 25.
    for (int i = 0; i < 6; ++i) {
        for (int j = 0; j < 3; ++j) {
            m.setFunction(m.dimension - 11 + j, i, false);
            m.setFunction(i, m.dimension - 11 + j, false);
        }
    }
}

void drawFunctionPatterns(Matrix &m, int version)
{
    drawFinder(m, 0, 0);
    drawFinder(m, m.dimension - 7, 0);
    drawFinder(m, 0, m.dimension - 7);
    drawTiming(m);

    const QList<int> centres = alignmentPositions(version);
    for (int r = 0; r < centres.size(); ++r) {
        for (int c = 0; c < centres.size(); ++c) {
            const int cx = centres.at(c);
            const int cy = centres.at(r);
            // Skip the three finder corners. ISO/IEC 18004 7.3.4.
            const bool topLeft = cx <= 8 && cy <= 8;
            const bool topRight = cx >= m.dimension - 9 && cy <= 8;
            const bool bottomLeft = cx <= 8 && cy >= m.dimension - 9;
            if (topLeft || topRight || bottomLeft) {
                continue;
            }
            drawAlignment(m, cx, cy);
        }
    }

    reserveFormatAreas(m);
    reserveVersionAreas(m, version);
}

// --- Data placement and masking --------------------------------------------

// ISO/IEC 18004 Table 24. i = row, j = column.
bool maskBit(int mask, int x, int y)
{
    const int i = y;
    const int j = x;
    switch (mask) {
    case 0:
        return ((i + j) % 2) == 0;
    case 1:
        return (i % 2) == 0;
    case 2:
        return (j % 3) == 0;
    case 3:
        return ((i + j) % 3) == 0;
    case 4:
        return ((i / 2 + j / 3) % 2) == 0;
    case 5:
        return ((i * j) % 2 + (i * j) % 3) == 0;
    case 6:
        return (((i * j) % 2 + (i * j) % 3) % 2) == 0;
    case 7:
        return (((i + j) % 2 + (i * j) % 3) % 2) == 0;
    default:
        return false;
    }
}

// Place data bits in the two-module-wide zigzag starting at the bottom-right
// column pair. ISO/IEC 18004 8.1 / Figure 14. Direction alternates per pair.
void placeData(Matrix &m, const QVector<quint8> &bits, int mask)
{
    int bitIndex = 0;
    bool upward = true;
    for (int right = m.dimension - 1; right >= 1; right -= 2) {
        if (right == 6) {
            // Column 6 is the vertical timing pattern; shift the pair left.
            --right;
        }
        for (int vert = 0; vert < m.dimension; ++vert) {
            const int y = upward ? (m.dimension - 1 - vert) : vert;
            for (int k = 0; k < 2; ++k) {
                const int x = right - k;
                if (m.function(x, y)) {
                    continue;
                }
                bool dark = false;
                if (bitIndex < bits.size()) {
                    dark = bits.at(bitIndex) != 0;
                    ++bitIndex;
                }
                if (maskBit(mask, x, y)) {
                    dark = !dark;
                }
                m.setModule(x, y, dark);
            }
        }
        upward = !upward;
    }
}

// --- Format and version information ----------------------------------------

// BCH(15,5) format information. ISO/IEC 18004 8.9 / Annex A.
quint16 formatBits(MdQrEcLevel ec, int mask)
{
    // EC level indicator bits: L = 0b01, M = 0b00. Table 12.
    const int ecBits = (ec == MdQrEcLevel::L) ? 0x1 : 0x0;
    quint32 data = quint32((ecBits << 3) | (mask & 0x7));
    quint32 rem = data;
    for (int i = 0; i < 10; ++i) {
        rem = (rem << 1) ^ (((rem >> 9) & 1u) ? 0x537u : 0u);
    }
    const quint16 bits = quint16(((data << 10) | (rem & 0x3FFu)) ^ 0x5412u);
    return bits;
}

// BCH(18,6) version information for versions 7-10. ISO/IEC 18004 8.10 / Annex D.
quint32 versionBits(int version)
{
    quint32 rem = quint32(version);
    for (int i = 0; i < 12; ++i) {
        rem = (rem << 1) ^ (((rem >> 11) & 1u) ? 0x1F25u : 0u);
    }
    return (quint32(version) << 12) | (rem & 0xFFFu);
}

void drawFormat(Matrix &m, MdQrEcLevel ec, int mask)
{
    const quint16 bits = formatBits(ec, mask);
    // Bit 0 is the LSB of the 15-bit format. Placement: ISO/IEC 18004 Figure 23.
    auto setFmt = [&m](int x, int y, bool dark) {
        if (x >= 0 && y >= 0 && x < m.dimension && y < m.dimension) {
            m.setFunction(x, y, dark);
        }
    };

    // Around the top-left finder. bits 0..14 mapped as in Figure 23.
    for (int i = 0; i <= 5; ++i) {
        setFmt(8, i, (bits >> i) & 1u);
    }
    setFmt(8, 7, (bits >> 6) & 1u);
    setFmt(8, 8, (bits >> 7) & 1u);
    setFmt(7, 8, (bits >> 8) & 1u);
    for (int i = 9; i <= 14; ++i) {
        setFmt(14 - i, 8, (bits >> i) & 1u);
    }

    // The split copy beside the other two finders.
    for (int i = 0; i <= 7; ++i) {
        setFmt(m.dimension - 1 - i, 8, (bits >> i) & 1u);
    }
    for (int i = 8; i <= 14; ++i) {
        setFmt(8, m.dimension - 15 + i, (bits >> i) & 1u);
    }
}

void drawVersion(Matrix &m, int version)
{
    if (version < 7) {
        return;
    }
    const quint32 bits = versionBits(version);
    // Bottom-left block first (bit 0 at bottom-left), then top-right.
    // ISO/IEC 18004 Figure 25.
    for (int i = 0; i < 18; ++i) {
        const bool dark = (bits >> i) & 1u;
        const int a = i / 3;
        const int b = i % 3;
        m.setFunction(b, m.dimension - 11 + a, dark);
        m.setFunction(m.dimension - 11 + a, b, dark);
    }
}

// --- Mask penalty -----------------------------------------------------------

int penaltyScore(const Matrix &m)
{
    int score = 0;
    const int n = m.dimension;

    // N1: adjacent modules in row/column in runs of 5+.
    // ISO/IEC 18004 Table 25, rule 1. Score = 3 + (run - 5).
    auto runPenalty = [](const QVector<quint8> &line) {
        int total = 0;
        if (line.isEmpty()) {
            return total;
        }
        int run = 1;
        for (int i = 1; i < line.size(); ++i) {
            if (line[i] == line[i - 1]) {
                ++run;
            } else {
                if (run >= 5) {
                    total += 3 + (run - 5);
                }
                run = 1;
            }
        }
        if (run >= 5) {
            total += 3 + (run - 5);
        }
        return total;
    };

    for (int y = 0; y < n; ++y) {
        QVector<quint8> row(n);
        for (int x = 0; x < n; ++x) {
            row[x] = m.modules[m.index(x, y)];
        }
        score += runPenalty(row);
    }
    for (int x = 0; x < n; ++x) {
        QVector<quint8> col(n);
        for (int y = 0; y < n; ++y) {
            col[y] = m.modules[m.index(x, y)];
        }
        score += runPenalty(col);
    }

    // N2: 2×2 blocks of the same colour. Score = 3 each. Table 25, rule 2.
    for (int y = 0; y < n - 1; ++y) {
        for (int x = 0; x < n - 1; ++x) {
            const quint8 a = m.modules[m.index(x, y)];
            if (a == m.modules[m.index(x + 1, y)]
                && a == m.modules[m.index(x, y + 1)]
                && a == m.modules[m.index(x + 1, y + 1)]) {
                score += 3;
            }
        }
    }

    // N3: 1:1:3:1:1 finder-like pattern with 4-module light on one side.
    // Score = 40 each. Table 25, rule 3.
    // Dark-light-dark-dark-dark-light-dark with 0000 on either side.
    const auto isFinderAt = [&m](const QVector<int> &coords, bool horizontal,
                                 int start) {
        // coords is 11 modules starting at `start` along the axis.
        static const int kPattern[11] = { 1, 0, 1, 1, 1, 0, 1, 0, 0, 0, 0 };
        static const int kReverse[11] = { 0, 0, 0, 0, 1, 0, 1, 1, 1, 0, 1 };
        auto at = [&](int k) {
            const int x = horizontal ? start + k : coords[0];
            const int y = horizontal ? coords[0] : start + k;
            return m.modules[m.index(x, y)] != 0;
        };
        bool forward = true;
        bool backward = true;
        for (int k = 0; k < 11; ++k) {
            if (at(k) != bool(kPattern[k])) {
                forward = false;
            }
            if (at(k) != bool(kReverse[k])) {
                backward = false;
            }
        }
        return forward || backward;
    };

    for (int y = 0; y < n; ++y) {
        QVector<int> rowRef(1, y);
        for (int x = 0; x + 11 <= n; ++x) {
            if (isFinderAt(rowRef, true, x)) {
                score += 40;
            }
        }
    }
    for (int x = 0; x < n; ++x) {
        QVector<int> colRef(1, x);
        for (int y = 0; y + 11 <= n; ++y) {
            if (isFinderAt(colRef, false, y)) {
                score += 40;
            }
        }
    }

    // N4: deviation of dark-module proportion from 50% in 5% steps.
    // Score = 10 × k. Table 25, rule 4.
    int dark = 0;
    for (quint8 bit : m.modules) {
        dark += bit;
    }
    const int total = n * n;
    const int percent = (dark * 100) / total;
    const int k = qAbs(percent - 50) / 5;
    score += 10 * k;

    return score;
}

// --- Data codewords --------------------------------------------------------

// Pack the byte-mode bit stream, pad to the data capacity and split into
// blocks with per-block Reed-Solomon EC, then interleave. ISO/IEC 18004 8.1
// and 8.2.
bool buildCodewords(const QByteArray &payload,
                    int version,
                    MdQrEcLevel ec,
                    QVector<quint8> *finalCodewords,
                    QString *error)
{
    const int dataCw = dataCodewords(version, ec);
    const int totalCw = totalCodewords(version);
    const int countBits = countBitsFor(version);
    const int capacityBits = dataCw * 8;

    BitBuffer bits;
    bits.append(0x4, 4); // byte mode indicator, ISO/IEC 18004 Table 1
    bits.append(quint32(payload.size()), countBits);
    for (int i = 0; i < payload.size(); ++i) {
        bits.append(quint32(uchar(payload.at(i))), 8);
    }
    if (bits.size() > capacityBits) {
        if (error) {
            *error = QStringLiteral("payload exceeds version %1 capacity")
                         .arg(version);
        }
        return false;
    }
    // Terminator: up to four 0 bits. ISO/IEC 18004 8.1.
    const int term = qMin(4, capacityBits - bits.size());
    bits.append(0, term);
    if (bits.size() % 8 != 0) {
        bits.append(0, 8 - (bits.size() % 8));
    }
    // Pad codewords 0xEC / 0x11 alternating. ISO/IEC 18004 Table 2.
    QVector<quint8> data = bits.toBytes();
    bool padEc = true;
    while (data.size() < dataCw) {
        data.append(padEc ? 0xEC : 0x11);
        padEc = !padEc;
    }
    data.resize(dataCw);

    const BlockPlan plan = blockPlan(version, ec);
    if (plan.g1Blocks <= 0 || plan.ecPerBlock <= 0) {
        if (error) {
            *error = QStringLiteral("missing EC block table for version %1")
                         .arg(version);
        }
        return false;
    }

    // Split into blocks.
    QVector<QVector<quint8>> dataBlocks;
    QVector<QVector<quint8>> ecBlocks;
    int offset = 0;
    const int totalBlocks = plan.g1Blocks + plan.g2Blocks;
    for (int b = 0; b < totalBlocks; ++b) {
        const int len = (b < plan.g1Blocks) ? plan.g1Data : plan.g2Data;
        QVector<quint8> block;
        block.reserve(len);
        for (int i = 0; i < len; ++i) {
            block.append(data.at(offset + i));
        }
        offset += len;
        dataBlocks.append(block);
        ecBlocks.append(rsRemainder(block, plan.ecPerBlock));
    }

    // Interleave. ISO/IEC 18004 8.2 / Figure 13.
    QVector<quint8> out;
    out.reserve(totalCw);
    const int maxData = qMax(plan.g1Data, plan.g2Data);
    for (int i = 0; i < maxData; ++i) {
        for (int b = 0; b < totalBlocks; ++b) {
            if (i < dataBlocks[b].size()) {
                out.append(dataBlocks[b][i]);
            }
        }
    }
    for (int i = 0; i < plan.ecPerBlock; ++i) {
        for (int b = 0; b < totalBlocks; ++b) {
            out.append(ecBlocks[b][i]);
        }
    }

    if (out.size() != totalCw) {
        if (error) {
            *error = QStringLiteral("interleave length mismatch (%1 != %2)")
                         .arg(out.size())
                         .arg(totalCw);
        }
        return false;
    }
    *finalCodewords = out;
    return true;
}

} // namespace

// --- Public API -------------------------------------------------------------

int MdQrEncoder::dimensionForVersion(int version)
{
    if (version < kMinVersion || version > kMaxVersion) {
        return 0;
    }
    return dimensionOf(version);
}

int MdQrEncoder::byteCapacity(int version, MdQrEcLevel ec)
{
    if (version < kMinVersion || version > kMaxVersion
        || ec == MdQrEcLevel::Count) {
        return -1;
    }
    const int dataCw = dataCodewords(version, ec);
    const int overhead = 4 + countBitsFor(version);
    return (dataCw * 8 - overhead) / 8;
}

MdQrEncodeResult MdQrEncoder::encode(const QByteArray &data,
                                     MdQrEcLevel ec,
                                     int minVersion,
                                     int maxVersion)
{
    MdQrEncodeResult result;
    if (ec != MdQrEcLevel::L && ec != MdQrEcLevel::M) {
        result.error = QStringLiteral("error-correction level out of scope (L/M only)");
        return result;
    }
    minVersion = qMax(minVersion, kMinVersion);
    maxVersion = qMin(maxVersion, kMaxVersion);
    if (minVersion > maxVersion) {
        result.error = QStringLiteral("empty version range");
        return result;
    }

    int version = -1;
    for (int v = minVersion; v <= maxVersion; ++v) {
        if (data.size() <= byteCapacity(v, ec)) {
            version = v;
            break;
        }
    }
    if (version < 0) {
        result.error = QStringLiteral("payload does not fit versions %1-%2 at EC %3")
                           .arg(minVersion)
                           .arg(maxVersion)
                           .arg(ec == MdQrEcLevel::L ? QStringLiteral("L")
                                                     : QStringLiteral("M"));
        return result;
    }

    QVector<quint8> codewords;
    if (!buildCodewords(data, version, ec, &codewords, &result.error)) {
        return result;
    }

    // Flatten codewords into a bit stream. ISO/IEC 18004 8.1.
    QVector<quint8> bits;
    bits.reserve(codewords.size() * 8);
    for (quint8 byte : codewords) {
        for (int i = 7; i >= 0; --i) {
            bits.append(quint8((byte >> i) & 1u));
        }
    }

    const int dim = dimensionOf(version);
    int bestMask = 0;
    int bestScore = 0;
    Matrix best;
    bool haveBest = false;

    for (int mask = 0; mask < 8; ++mask) {
        Matrix m;
        m.init(dim);
        drawFunctionPatterns(m, version);
        placeData(m, bits, mask);
        drawFormat(m, ec, mask);
        drawVersion(m, version);
        const int score = penaltyScore(m);
        if (!haveBest || score < bestScore) {
            haveBest = true;
            bestScore = score;
            bestMask = mask;
            best = m;
        }
    }

    Q_UNUSED(bestMask);
    result.ok = true;
    result.version = version;
    result.dimension = dim;
    result.modules.clear();
    result.modules.reserve(dim * dim);
    for (int y = 0; y < dim; ++y) {
        for (int x = 0; x < dim; ++x) {
            result.modules.append(best.module(x, y));
        }
    }
    return result;
}

MdQrEncodeResult MdQrEncoder::encodeText(const QString &text,
                                         MdQrEcLevel ec,
                                         int minVersion,
                                         int maxVersion)
{
    return encode(text.toUtf8(), ec, minVersion, maxVersion);
}

} // namespace md
