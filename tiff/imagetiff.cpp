#include "imagetiff.h"
#include <QIODevice>
#include <QImage>
#include <QFile>
#include <QDebug>
extern "C" {
#include "tiffio.h"
}

tsize_t qtiffReadProc(thandle_t fd, tdata_t buf, tsize_t size)
{
    QIODevice *device = static_cast<QIODevice *>(fd);
    return device->isReadable() ? device->read(static_cast<char *>(buf), size) : -1;
}

tsize_t qtiffWriteProc(thandle_t fd, tdata_t buf, tsize_t size)
{
    return static_cast<QIODevice *>(fd)->write(static_cast<char *>(buf), size);
}

toff_t qtiffSeekProc(thandle_t fd, toff_t off, int whence)
{
    QIODevice *device = static_cast<QIODevice *>(fd);
    switch (whence) {
    case SEEK_SET:
        device->seek(off);
        break;
    case SEEK_CUR:
        device->seek(device->pos() + off);
        break;
    case SEEK_END:
        device->seek(device->size() + off);
        break;
    }

    return device->pos();
}

int qtiffCloseProc(thandle_t /*fd*/)
{
    return 0;
}

toff_t qtiffSizeProc(thandle_t fd)
{
    return static_cast<QIODevice *>(fd)->size();
}

int qtiffMapProc(thandle_t /*fd*/, tdata_t* /*pbase*/, toff_t* /*psize*/)
{
    return 0;
}

void qtiffUnmapProc(thandle_t /*fd*/, tdata_t /*base*/, toff_t /*size*/)
{
}

ImageTiff::ImageTiff(QObject *parent) : QObject(parent)
{

}

ImageTiff::~ImageTiff()
{

}

static bool checkGrayscale(const QVector<QRgb> &colorTable)
{
    if (colorTable.size() != 256)
        return false;

    const bool increasing = (colorTable.at(0) == 0xff000000);
    for (int i = 0; i < 256; ++i) {
        if ((increasing && colorTable.at(i) != qRgb(i, i, i))
            || (!increasing && colorTable.at(i) != qRgb(255 - i, 255 - i, 255 - i)))
            return false;
    }
    return true;
}

static QVector<QRgb> effectiveColorTable(const QImage &image)
{
    QVector<QRgb> colors;
    switch (image.format()) {
    case QImage::Format_Indexed8:
        colors = image.colorTable();
        break;
//    case QImage::Format_Alpha8:
//        colors.resize(256);
//        for (int i = 0; i < 256; ++i)
//            colors[i] = qRgba(0, 0, 0, i);
//        break;
//    case QImage::Format_Grayscale8:
//        colors.resize(256);
//        for (int i = 0; i < 256; ++i)
//            colors[i] = qRgb(i, i, i);
//        break;
    default:
        Q_UNREACHABLE();
    }
    return colors;
}

bool ImageTiff::write(QString filePath, const QImage &image)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadWrite | QIODevice::Truncate)) {
        qWarning() << "File" << filePath << "couldn't be open for writing";
        return false;
    }

    TIFF *const tiff = TIFFClientOpen("foo",
                                      "wB",
                                      &file,
                                      qtiffReadProc,
                                      qtiffWriteProc,
                                      qtiffSeekProc,
                                      qtiffCloseProc,
                                      qtiffSizeProc,
                                      qtiffMapProc,
                                      qtiffUnmapProc);
    if (!tiff)
        return false;

    const int width = image.width();
    const int height = image.height();
    const int compression = NoCompression;
    const int exifTagTransormation = 1; // QImageIOHandler::TransformationNone -> 1

    if (!TIFFSetField(tiff, TIFFTAG_IMAGEWIDTH, width)
        || !TIFFSetField(tiff, TIFFTAG_IMAGELENGTH, height)
        || !TIFFSetField(tiff, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG)) {
        TIFFClose(tiff);
        return false;
    }

    // set the resolution
    bool  resolutionSet = false;
    const int dotPerMeterX = image.dotsPerMeterX();
    const int dotPerMeterY = image.dotsPerMeterY();
    if ((dotPerMeterX % 100) == 0
        && (dotPerMeterY % 100) == 0) {
        resolutionSet = TIFFSetField(tiff, TIFFTAG_RESOLUTIONUNIT, RESUNIT_CENTIMETER)
                        && TIFFSetField(tiff, TIFFTAG_XRESOLUTION, dotPerMeterX/100.0)
                        && TIFFSetField(tiff, TIFFTAG_YRESOLUTION, dotPerMeterY/100.0);
    } else {
        resolutionSet = TIFFSetField(tiff, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH)
                        && TIFFSetField(tiff, TIFFTAG_XRESOLUTION, static_cast<float>(image.logicalDpiX()))
                        && TIFFSetField(tiff, TIFFTAG_YRESOLUTION, static_cast<float>(image.logicalDpiY()));
    }
    if (!resolutionSet) {
        TIFFClose(tiff);
        return false;
    }
    // set the orienataion
    bool orientationSet = false;
    orientationSet = TIFFSetField(tiff, TIFFTAG_ORIENTATION, exifTagTransormation);
    if (!orientationSet) {
        TIFFClose(tiff);
        return false;
    }

    // configure image depth
    const QImage::Format format = image.format();
    if (format == QImage::Format_Mono || format == QImage::Format_MonoLSB) {
        uint16 photometric = PHOTOMETRIC_MINISBLACK;
        if (image.colorTable().at(0) == 0xffffffff)
            photometric = PHOTOMETRIC_MINISWHITE;
        if (!TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, photometric)
            || !TIFFSetField(tiff, TIFFTAG_COMPRESSION, compression == NoCompression ? COMPRESSION_NONE : COMPRESSION_CCITTRLE)
            || !TIFFSetField(tiff, TIFFTAG_BITSPERSAMPLE, 1)) {
            TIFFClose(tiff);
            return false;
        }

        // try to do the conversion in chunks no greater than 16 MB
        int chunks = (width * height / (1024 * 1024 * 16)) + 1;
        int chunkHeight = qMax(height / chunks, 1);

        int y = 0;
        while (y < height) {
            QImage chunk = image.copy(0, y, width, qMin(chunkHeight, height - y)).convertToFormat(QImage::Format_Mono);

            int chunkStart = y;
            int chunkEnd = y + chunk.height();
            while (y < chunkEnd) {
                if (TIFFWriteScanline(tiff, reinterpret_cast<uint32 *>(chunk.scanLine(y - chunkStart)), y) != 1) {
                    TIFFClose(tiff);
                    return false;
                }
                ++y;
            }
        }
        TIFFClose(tiff);
    } else if (format == QImage::Format_Indexed8) {
//               || format == QImage::Format_Grayscale8
//               || format == QImage::Format_Alpha8) {
        QVector<QRgb> colorTable = effectiveColorTable(image);
        bool isGrayscale = checkGrayscale(colorTable);
        if (isGrayscale) {
            uint16 photometric = PHOTOMETRIC_MINISBLACK;
            if (colorTable.at(0) == 0xffffffff)
                photometric = PHOTOMETRIC_MINISWHITE;
            if (!TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, photometric)
                    || !TIFFSetField(tiff, TIFFTAG_COMPRESSION, compression == NoCompression ? COMPRESSION_NONE : COMPRESSION_PACKBITS)
                    || !TIFFSetField(tiff, TIFFTAG_BITSPERSAMPLE, 8)) {
                TIFFClose(tiff);
                return false;
            }
        } else {
            if (!TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_PALETTE)
                    || !TIFFSetField(tiff, TIFFTAG_COMPRESSION, compression == NoCompression ? COMPRESSION_NONE : COMPRESSION_PACKBITS)
                    || !TIFFSetField(tiff, TIFFTAG_BITSPERSAMPLE, 8)) {
                TIFFClose(tiff);
                return false;
            }
            //// write the color table
            // allocate the color tables
            const int tableSize = colorTable.size();
            Q_ASSERT(tableSize <= 256);
            QVarLengthArray<uint16> redTable(tableSize);
            QVarLengthArray<uint16> greenTable(tableSize);
            QVarLengthArray<uint16> blueTable(tableSize);

            // set the color table
            for (int i = 0; i<tableSize; ++i) {
                const QRgb color = colorTable.at(i);
                redTable[i] = qRed(color) * 257;
                greenTable[i] = qGreen(color) * 257;
                blueTable[i] = qBlue(color) * 257;
            }

            const bool setColorTableSuccess = TIFFSetField(tiff, TIFFTAG_COLORMAP, redTable.data(), greenTable.data(), blueTable.data());

            if (!setColorTableSuccess) {
                TIFFClose(tiff);
                return false;
            }
        }

        //// write the data
        // try to do the conversion in chunks no greater than 16 MB
        int chunks = (width * height/ (1024 * 1024 * 16)) + 1;
        int chunkHeight = qMax(height / chunks, 1);

        int y = 0;
        while (y < height) {
            QImage chunk = image.copy(0, y, width, qMin(chunkHeight, height - y));

            int chunkStart = y;
            int chunkEnd = y + chunk.height();
            while (y < chunkEnd) {
                if (TIFFWriteScanline(tiff, reinterpret_cast<uint32 *>(chunk.scanLine(y - chunkStart)), y) != 1) {
                    TIFFClose(tiff);
                    return false;
                }
                ++y;
            }
        }
        TIFFClose(tiff);
    } else if (!image.hasAlphaChannel()) {
        if (!TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB)
            || !TIFFSetField(tiff, TIFFTAG_COMPRESSION, compression == NoCompression ? COMPRESSION_NONE : COMPRESSION_LZW)
            || !TIFFSetField(tiff, TIFFTAG_SAMPLESPERPIXEL, 3)
            || !TIFFSetField(tiff, TIFFTAG_BITSPERSAMPLE, 8)) {
            TIFFClose(tiff);
            return false;
        }
        // try to do the RGB888 conversion in chunks no greater than 16 MB
        const int chunks = (width * height * 3 / (1024 * 1024 * 16)) + 1;
        const int chunkHeight = qMax(height / chunks, 1);

        int y = 0;
        while (y < height) {
            const QImage chunk = image.copy(0, y, width, qMin(chunkHeight, height - y)).convertToFormat(QImage::Format_RGB888);

            int chunkStart = y;
            int chunkEnd = y + chunk.height();
            while (y < chunkEnd) {
                if (TIFFWriteScanline(tiff, (void*)chunk.scanLine(y - chunkStart), y) != 1) {
                    TIFFClose(tiff);
                    return false;
                }
                ++y;
            }
        }
        TIFFClose(tiff);
    } else {
        const bool premultiplied = image.format() != QImage::Format_ARGB32
                                && image.format() != QImage::Format_RGBA8888;
        const uint16 extrasamples = premultiplied ? EXTRASAMPLE_ASSOCALPHA : EXTRASAMPLE_UNASSALPHA;
        if (!TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB)
            || !TIFFSetField(tiff, TIFFTAG_COMPRESSION, compression == NoCompression ? COMPRESSION_NONE : COMPRESSION_LZW)
            || !TIFFSetField(tiff, TIFFTAG_SAMPLESPERPIXEL, 4)
            || !TIFFSetField(tiff, TIFFTAG_BITSPERSAMPLE, 8)
            || !TIFFSetField(tiff, TIFFTAG_EXTRASAMPLES, 1, &extrasamples)) {
            TIFFClose(tiff);
            return false;
        }
        // try to do the RGBA8888 conversion in chunks no greater than 16 MB
        const int chunks = (width * height * 4 / (1024 * 1024 * 16)) + 1;
        const int chunkHeight = qMax(height / chunks, 1);

        const QImage::Format format = premultiplied ? QImage::Format_RGBA8888_Premultiplied
                                                    : QImage::Format_RGBA8888;
        int y = 0;
        while (y < height) {
            const QImage chunk = image.copy(0, y, width, qMin(chunkHeight, height - y)).convertToFormat(format);

            int chunkStart = y;
            int chunkEnd = y + chunk.height();
            while (y < chunkEnd) {
                if (TIFFWriteScanline(tiff, (void*)chunk.scanLine(y - chunkStart), y) != 1) {
                    TIFFClose(tiff);
                    return false;
                }
                ++y;
            }
        }
        TIFFClose(tiff);
    }

    return true;
}
