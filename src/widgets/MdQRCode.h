#ifndef MD_QR_CODE_H
#define MD_QR_CODE_H

// MdQRCode — Stage 2 Qt extension (Appendix D, P2).
//
// Semantic anchor (appendix D): "Surface container + outline 描边（如需）".
//
// A matrix-visual QR display: a surface-container card whose centre holds a
// square module grid drawn from the widget's ISO/IEC 18004 encoder (or from a
// caller-supplied matrix).
//
//   container surface-container, ShapeCorner::Medium
//   modules   on-surface squares on surface (the QR's dark/light pair)
//   quiet     a module-scale quiet zone the spec requires (4 modules)
//
// MD3 has no QR family; the control is the surface reading of any 2-D code.
// `setText()` encodes the payload with `MdQrEncoder` — byte mode, versions
// 1-10, error correction L/M/Q/H (see `core/MdQrEncoder.h` for the subset and the
// ISO/IEC 18004 citations). A payload that does not fit leaves the previous
// matrix in place; `setMatrix()` still accepts a pre-encoded grid verbatim.
//
// There is no QSS.

#include "core/MdQrEncoder.h"
#include "core/MdQtCompat.h"
#include "core/MdTypes.h"
#include "core/QtMd3Export.h"

#include <QtCore/QList>
#include <QtCore/QString>
#include <QtWidgets/QWidget>

class QPaintEvent;

namespace md {

class QT_MD3_EXPORT MdQRCode : public QWidget
{
    Q_OBJECT

    /// Payload text. Setting it regenerates the module matrix through the
    /// ISO/IEC 18004 encoder (see the header note on the supported subset).
    Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
    /// Edge of one module, in logical px (default 4).
    Q_PROPERTY(int moduleSize READ moduleSize WRITE setModuleSize NOTIFY moduleSizeChanged)
    /// When true a quiet zone of 4 modules is kept around the matrix.
    Q_PROPERTY(bool quietZone READ hasQuietZone WRITE setQuietZone NOTIFY quietZoneChanged)
    /// Error-correction level used by setText (default M).
    Q_PROPERTY(md::MdQrEcLevel ecLevel READ ecLevel WRITE setEcLevel NOTIFY ecLevelChanged)

public:
    explicit MdQRCode(QWidget *parent = nullptr);
    explicit MdQRCode(const QString &text, QWidget *parent = nullptr);
    ~MdQRCode() override;

    QString text() const { return m_text; }
    void setText(const QString &text);

    int moduleSize() const { return m_moduleSize; }
    void setModuleSize(int size);

    bool hasQuietZone() const { return m_quietZone; }
    void setQuietZone(bool on);

    MdQrEcLevel ecLevel() const { return m_ecLevel; }
    void setEcLevel(MdQrEcLevel level);

    /// Side length of the module matrix (modules per edge). 0 when empty.
    int matrixDimension() const { return m_dimension; }

    /// Replace the module matrix. `dimension` is the side length; `modules`
    /// is row-major, one bool per cell (true = dark). The list must hold
    /// dimension*dimension entries or the call is ignored.
    void setMatrix(int dimension, const QList<bool> &modules);
    void clearMatrix();

    /// One module's dark/light state. Out-of-range reads return false.
    bool moduleAt(int x, int y) const;

    /// Resolved module (dark) colour: on-surface.
    QColor resolvedModuleColor() const;
    /// Resolved background colour: surface-container.
    QColor resolvedSurfaceColor() const;

    /// The painted code area (matrix + quiet zone), in widget coordinates.
    QRectF codeRect() const;

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void textChanged(const QString &text);
    void moduleSizeChanged(int size);
    void quietZoneChanged(bool on);
    void ecLevelChanged(md::MdQrEcLevel level);
    void matrixChanged(int dimension);

protected:
    void paintEvent(QPaintEvent *event) override;
    void changeEvent(QEvent *event) override;

private slots:
    void onThemeChanged();

private:
    void init();
    void regenerate();

    QString m_text;
    int m_moduleSize = 4;
    bool m_quietZone = true;
    MdQrEcLevel m_ecLevel = MdQrEcLevel::M;
    int m_dimension = 0;
    QList<bool> m_modules;
};

} // namespace md

#endif // MD_QR_CODE_H
