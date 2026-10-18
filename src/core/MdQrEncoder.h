#ifndef MD_QR_ENCODER_H
#define MD_QR_ENCODER_H

// MdQrEncoder — minimal ISO/IEC 18004 QR Code encoder.
//
// Scope (deliberate subset, recorded in docs/porting-todo.md):
//   * Byte mode only (UTF-8 payload, mode indicator 0b0100).
//   * Versions 1-10 (21×21 … 57×57 modules).
//   * Error correction levels L (~7%), M (~15%), Q (~25%) and H (~30%).
//
// Everything the matrix needs is produced here: data codeword packing,
// Reed-Solomon error correction over GF(256) with the 0x11D primitive
// polynomial, block interleaving, function patterns (finders, separators,
// timing, alignment, dark module), format / version information, the eight
// data masks with the ISO penalty evaluation, and the final module matrix.
//
// Standard reference: ISO/IEC 18004:2015 (QR Code), sections 7-8 and the
// annex tables for capacity, EC blocks and generator polynomials. The bit
// packing, placement zigzag, mask predicates and penalty rules follow the
// published figures; numeric values are transcribed from those tables rather
// than estimated.
//
// There is no QSS and no Qt Widgets dependency — this is a pure value-type
// encoder so it can be unit-tested without a window.

#include "QtMd3Export.h"

#include <QtCore/QByteArray>
#include <QtCore/QMetaType>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QtGlobal>

namespace md {

/// QR error-correction level (ISO/IEC 18004 Table 12 indicator bits).
enum class MdQrEcLevel {
    L = 0, ///< ~7% recovery
    M = 1, ///< ~15% recovery
    Q = 2, ///< ~25% recovery
    H = 3, ///< ~30% recovery
    Count
};

/// Result of `MdQrEncoder::encode`. `modules` is row-major, true = dark,
/// `dimension` × `dimension` entries when `ok`.
struct QT_MD3_EXPORT MdQrEncodeResult {
    bool ok = false;
    int version = 0;
    int dimension = 0;
    QList<bool> modules;
    QString error;
};

class QT_MD3_EXPORT MdQrEncoder
{
public:
    /// Encode `data` as a QR symbol. Picks the smallest version in
    /// [minVersion, maxVersion] whose byte-mode capacity holds the payload.
    /// Versions outside 1-10 are rejected (this module's scope).
    static MdQrEncodeResult encode(const QByteArray &data,
                                   MdQrEcLevel ec = MdQrEcLevel::M,
                                   int minVersion = 1,
                                   int maxVersion = 10);

    /// Convenience wrapper: UTF-8 encodes `text` then calls `encode`.
    static MdQrEncodeResult encodeText(const QString &text,
                                       MdQrEcLevel ec = MdQrEcLevel::M,
                                       int minVersion = 1,
                                       int maxVersion = 10);

    /// Side length in modules of `version` (17 + 4·version). 0 when invalid.
    static int dimensionForVersion(int version);

    /// Maximum UTF-8 byte payload that fits `version` at `ec` in byte mode.
    /// Negative when the version is out of the 1-10 scope.
    static int byteCapacity(int version, MdQrEcLevel ec);
};

} // namespace md

Q_DECLARE_METATYPE(md::MdQrEcLevel)

#endif // MD_QR_ENCODER_H
