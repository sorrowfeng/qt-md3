#ifndef TEST_MD3_COMMON_H
#define TEST_MD3_COMMON_H

// Shared helpers for the qt-md3 test binaries.
//
// Deliberately a header rather than a compiled support library: the helpers are
// three lines each, and keeping them inline means a test binary has no
// dependency beyond the library under test.

#include <QtGui/QImage>
#include <QtGui/QRgb>

namespace mdtest {

/// Number of pixels that differ from `background`. Used by the render smoke
/// checks, where the point is that *something* was drawn — a style that
/// silently paints nothing must not pass.
inline int paintedPixelCount(const QImage &image, QRgb background)
{
    int count = 0;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (image.pixel(x, y) != background) {
                ++count;
            }
        }
    }
    return count;
}

} // namespace mdtest

#endif // TEST_MD3_COMMON_H
