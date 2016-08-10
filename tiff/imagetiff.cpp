#include "imagetiff.h"
#include <QIODevice>
#include <QImage>
#include <QFile>
#include <QDebug>
extern "C" {
#include "tiffio.h"
}

#define TIFFTAG_FILEFORMAT        59998
#define TIFFTAG_TESTPOINTS_COUNT  59999
#define TIFFTAG_TESTPOINTS        60000

#define MAX_TESTPOINTS  1000

static char DEFAULT_TAG_NAME[] = "custom_tag";

static TIFFExtendProc parent_extender = NULL;  // In case we want a chain of extensions

static void registerCustomTIFFTags(TIFF *tif)
{
    /* Install the extended Tag field info */
    TIFFFieldInfo *tiffFieldInfos = (TIFFFieldInfo *)calloc(2 + MAX_TESTPOINTS, sizeof(TIFFFieldInfo));
    tiffFieldInfos[0].field_tag = TIFFTAG_FILEFORMAT;
    tiffFieldInfos[0].field_readcount = TIFF_VARIABLE;
    tiffFieldInfos[0].field_writecount = TIFF_VARIABLE;
    tiffFieldInfos[0].field_type = TIFF_ASCII;
    tiffFieldInfos[0].field_bit = FIELD_CUSTOM;
    tiffFieldInfos[0].field_oktochange = 1;
    tiffFieldInfos[0].field_passcount = 0;
    tiffFieldInfos[0].field_name = DEFAULT_TAG_NAME;
    tiffFieldInfos[1].field_tag = TIFFTAG_TESTPOINTS_COUNT;
    tiffFieldInfos[1].field_readcount = 1;
    tiffFieldInfos[1].field_writecount = 1;
    tiffFieldInfos[1].field_type = TIFF_SHORT;
    tiffFieldInfos[1].field_bit = FIELD_CUSTOM;
    tiffFieldInfos[1].field_oktochange = 1;
    tiffFieldInfos[1].field_passcount = 0;
    tiffFieldInfos[1].field_name = DEFAULT_TAG_NAME;
    for (int i = 0; i < MAX_TESTPOINTS; i++) {
        tiffFieldInfos[2 + i].field_tag = TIFFTAG_FILEFORMAT + i;
        tiffFieldInfos[2 + i].field_readcount = TIFF_VARIABLE;
        tiffFieldInfos[2 + i].field_writecount = TIFF_VARIABLE;
        tiffFieldInfos[2 + i].field_type = TIFF_ASCII;
        tiffFieldInfos[2 + i].field_bit = FIELD_CUSTOM;
        tiffFieldInfos[2 + i].field_oktochange = 1;
        tiffFieldInfos[2 + i].field_passcount = 0;
        tiffFieldInfos[2 + i].field_name = DEFAULT_TAG_NAME;
    }
    int error = TIFFMergeFieldInfo(tif, tiffFieldInfos, 2 + MAX_TESTPOINTS);
    if (error != 0) {
        qWarning() << "Custom tags couldn't be installed";
        Q_ASSERT(false);
    }

    if (parent_extender)
        (*parent_extender)(tif);
}

static void augment_libtiff_with_custom_tags() {
    static bool first_time = true;
    if (!first_time) return;
    first_time = false;
    parent_extender = TIFFSetTagExtender(registerCustomTIFFTags);
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

    // Create the TIFF directory object:
    augment_libtiff_with_custom_tags();

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

    // ... and now our own custom ones:
    TIFFSetField(tiff, TIFFTAG_FILEFORMAT, "V1.0");
    TIFFSetField(tiff, TIFFTAG_TESTPOINTS_COUNT, 1);
    TIFFSetField(tiff, TIFFTAG_TESTPOINTS+10, "POINT:10, X:232, Y:6522, SIG:sin, FREQ:1000, VOLT:2.6");

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

bool ImageTiff::writeImageSeries(QString filePath, QList<QImage> images)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadWrite | QIODevice::Truncate)) {
        qWarning() << "File" << filePath << "couldn't be open for writing";
        Q_ASSERT(false);
        return false;
    }

    // Create the TIFF directory object:
    augment_libtiff_with_custom_tags();

    m_tiff = TIFFClientOpen("foo", "wB", &file,
                            qtiffReadProc, qtiffWriteProc, qtiffSeekProc,
                            qtiffCloseProc, qtiffSizeProc, qtiffMapProc,
                            qtiffUnmapProc);
    if (!m_tiff) {
        qWarning() << "File" << filePath << "couldn't be opened for store TIFF image";
        Q_ASSERT(false);
        return false;
    }

    for (int page = 0; page < images.count(); page ++) {
        const QImage &image = images.at(page);
        TIFFSetField(m_tiff, TIFFTAG_SUBFILETYPE, FILETYPE_PAGE);
        TIFFSetField(m_tiff, TIFFTAG_PAGENUMBER, page, images.count());
        if (!appendImage(image)) {
            qWarning() << "Image" << image << "couldn't be written in" << filePath;
            TIFFClose(m_tiff);
            Q_ASSERT(false);
            return false;
        }
        TIFFWriteDirectory(m_tiff);
    }
    TIFFClose(m_tiff);
    qDebug() << "Tiff image has been successfully written to" << filePath;
    return true;
}

bool ImageTiff::appendImage(const QImage &image)
{
    const int width = image.width();
    const int height = image.height();
    const int compression = NoCompression;
    const int exifTagTransormation = 1; // QImageIOHandler::TransformationNone -> 1

    if (!TIFFSetField(m_tiff, TIFFTAG_IMAGEWIDTH, width)
        || !TIFFSetField(m_tiff, TIFFTAG_IMAGELENGTH, height)
        || !TIFFSetField(m_tiff, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG)) {
        qWarning() << "Can't set image's size:" << image;
        Q_ASSERT(false);
        return false;
    }

    // set the resolution
    bool  resolutionSet = false;
    const int dotPerMeterX = image.dotsPerMeterX();
    const int dotPerMeterY = image.dotsPerMeterY();
    if ((dotPerMeterX % 100) == 0
        && (dotPerMeterY % 100) == 0) {
        resolutionSet = TIFFSetField(m_tiff, TIFFTAG_RESOLUTIONUNIT, RESUNIT_CENTIMETER)
                        && TIFFSetField(m_tiff, TIFFTAG_XRESOLUTION, dotPerMeterX/100.0)
                        && TIFFSetField(m_tiff, TIFFTAG_YRESOLUTION, dotPerMeterY/100.0);
    } else {
        resolutionSet = TIFFSetField(m_tiff, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH)
                        && TIFFSetField(m_tiff, TIFFTAG_XRESOLUTION, static_cast<float>(image.logicalDpiX()))
                        && TIFFSetField(m_tiff, TIFFTAG_YRESOLUTION, static_cast<float>(image.logicalDpiY()));
    }
    if (!resolutionSet) {
        qWarning() << "Can't set image's resolution:" << image;
        Q_ASSERT(false);
        return false;
    }
    // set the orienataion
    bool orientationSet = false;
    orientationSet = TIFFSetField(m_tiff, TIFFTAG_ORIENTATION, exifTagTransormation);
    if (!orientationSet) {
        qWarning() << "Can't set image's orientation:" << image;
        Q_ASSERT(false);
        return false;
    }

    // ... and now our own custom ones:
    TIFFSetField(m_tiff, TIFFTAG_FILEFORMAT, "V1.0");
    TIFFSetField(m_tiff, TIFFTAG_TESTPOINTS_COUNT, 1);
    TIFFSetField(m_tiff, TIFFTAG_TESTPOINTS+10, "POINT:10, X:232, Y:6522, SIG:sin, FREQ:1000, VOLT:2.6");

    // configure image depth
    const QImage::Format format = image.format();
    if (format == QImage::Format_Mono || format == QImage::Format_MonoLSB) {
        qDebug() << "Image" << image << "have MONO format";
        uint16 photometric = PHOTOMETRIC_MINISBLACK;
        if (image.colorTable().at(0) == 0xffffffff)
            photometric = PHOTOMETRIC_MINISWHITE;
        if (!TIFFSetField(m_tiff, TIFFTAG_PHOTOMETRIC, photometric)
            || !TIFFSetField(m_tiff, TIFFTAG_COMPRESSION, compression == NoCompression ? COMPRESSION_NONE : COMPRESSION_CCITTRLE)
            || !TIFFSetField(m_tiff, TIFFTAG_BITSPERSAMPLE, 1)) {
            qWarning() << "Can't set image's format:" << image;
            Q_ASSERT(false);
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
                if (TIFFWriteScanline(m_tiff, reinterpret_cast<uint32 *>(chunk.scanLine(y - chunkStart)), y) != 1) {
                    qWarning() << "Can't write data in TIFF from image:" << image;
                    Q_ASSERT(false);
                    return false;
                }
                ++y;
            }
        }
        return true;
    }
    if (format == QImage::Format_Indexed8) {
//               || format == QImage::Format_Grayscale8
//               || format == QImage::Format_Alpha8) {
        qDebug() << "Image" << image << "have 8-bit format";
        QVector<QRgb> colorTable = effectiveColorTable(image);
        bool isGrayscale = checkGrayscale(colorTable);
        if (isGrayscale) {
            uint16 photometric = PHOTOMETRIC_MINISBLACK;
            if (colorTable.at(0) == 0xffffffff)
                photometric = PHOTOMETRIC_MINISWHITE;
            if (!TIFFSetField(m_tiff, TIFFTAG_PHOTOMETRIC, photometric)
                    || !TIFFSetField(m_tiff, TIFFTAG_COMPRESSION, compression == NoCompression ? COMPRESSION_NONE : COMPRESSION_PACKBITS)
                    || !TIFFSetField(m_tiff, TIFFTAG_BITSPERSAMPLE, 8)) {
                qWarning() << "Can't set image's format:" << image;
                Q_ASSERT(false);
                return false;
            }
        } else {
            if (!TIFFSetField(m_tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_PALETTE)
                    || !TIFFSetField(m_tiff, TIFFTAG_COMPRESSION, compression == NoCompression ? COMPRESSION_NONE : COMPRESSION_PACKBITS)
                    || !TIFFSetField(m_tiff, TIFFTAG_BITSPERSAMPLE, 8)) {
                qWarning() << "Can't set image's format:" << image;
                Q_ASSERT(false);
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

            const bool setColorTableSuccess = TIFFSetField(m_tiff, TIFFTAG_COLORMAP, redTable.data(), greenTable.data(), blueTable.data());

            if (!setColorTableSuccess) {
                qWarning() << "Can't set image's color table:" << image;
                Q_ASSERT(false);
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
                if (TIFFWriteScanline(m_tiff, reinterpret_cast<uint32 *>(chunk.scanLine(y - chunkStart)), y) != 1) {
                    qWarning() << "Can't write data in TIFF from image:" << image;
                    Q_ASSERT(false);
                    return false;
                }
                ++y;
            }
        }
        return true;
    }
    if (!image.hasAlphaChannel()) {
        qDebug() << "Image" << image << "have 24-bit format (no alpha channel)";
        if (!TIFFSetField(m_tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB)
            || !TIFFSetField(m_tiff, TIFFTAG_COMPRESSION, compression == NoCompression ? COMPRESSION_NONE : COMPRESSION_LZW)
            || !TIFFSetField(m_tiff, TIFFTAG_SAMPLESPERPIXEL, 3)
            || !TIFFSetField(m_tiff, TIFFTAG_BITSPERSAMPLE, 8)) {
            qWarning() << "Can't set store image's format:" << image;
            Q_ASSERT(false);
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
                if (TIFFWriteScanline(m_tiff, (void*)chunk.scanLine(y - chunkStart), y) != 1) {
                    qWarning() << "Can't write data in TIFF from image:" << image;
                    Q_ASSERT(false);
                    return false;
                }
                ++y;
            }
        }
        return true;
    }

    qDebug() << "Image" << image << "have 32-bit format (alpha channel presents)";
    const bool premultiplied = image.format() != QImage::Format_ARGB32
                            && image.format() != QImage::Format_RGBA8888;
    const uint16 extrasamples = premultiplied ? EXTRASAMPLE_ASSOCALPHA : EXTRASAMPLE_UNASSALPHA;
    if (!TIFFSetField(m_tiff, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB)
        || !TIFFSetField(m_tiff, TIFFTAG_COMPRESSION, compression == NoCompression ? COMPRESSION_NONE : COMPRESSION_LZW)
        || !TIFFSetField(m_tiff, TIFFTAG_SAMPLESPERPIXEL, 4)
        || !TIFFSetField(m_tiff, TIFFTAG_BITSPERSAMPLE, 8)
        || !TIFFSetField(m_tiff, TIFFTAG_EXTRASAMPLES, 1, &extrasamples)) {
        qWarning() << "Can't set store image's format:" << image;
        Q_ASSERT(false);
        return false;
    }
    // try to do the RGBA8888 conversion in chunks no greater than 16 MB
    const int chunks = (width * height * 4 / (1024 * 1024 * 16)) + 1;
    const int chunkHeight = qMax(height / chunks, 1);

    const QImage::Format format32 = premultiplied ? QImage::Format_RGBA8888_Premultiplied
                                                : QImage::Format_RGBA8888;
    int y = 0;
    while (y < height) {
        const QImage chunk = image.copy(0, y, width, qMin(chunkHeight, height - y)).convertToFormat(format32);

        int chunkStart = y;
        int chunkEnd = y + chunk.height();
        while (y < chunkEnd) {
            if (TIFFWriteScanline(m_tiff, (void*)chunk.scanLine(y - chunkStart), y) != 1) {
                qWarning() << "Can't write data in TIFF from image:" << image;
                Q_ASSERT(false);
                return false;
            }
            ++y;
        }
    }
    return true;
}
