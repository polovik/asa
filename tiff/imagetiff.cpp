#include "imagetiff.h"
#include <QIODevice>
#include <QImage>
#include <QFile>
#include <QDebug>
extern "C" {
#include "tiffio.h"
}

#define	TIFFFieldInfo_LEN(a)	(sizeof (a) / sizeof (a[0]))

#define TIFFTAG_FILEFORMAT              59998
#define TIFFTAG_TESTPOINT_DESCRIPTION   59999
#define TIFFTAG_TESTPOINT_DATA_X        60000
#define TIFFTAG_TESTPOINT_DATA_Y        60001

static TIFFExtendProc parent_extender = nullptr;  // In case we want a chain of extensions
static const TIFFFieldInfo xtiffFieldInfo[] = {
    { TIFFTAG_FILEFORMAT,           TIFF_VARIABLE, TIFF_VARIABLE, TIFF_ASCII,	FIELD_CUSTOM, 1,	0,	(char *)"FileFormat" },
    { TIFFTAG_TESTPOINT_DESCRIPTION,TIFF_VARIABLE, TIFF_VARIABLE, TIFF_ASCII,	FIELD_CUSTOM, 1,	0,	(char *)"Description" },
    { TIFFTAG_TESTPOINT_DATA_X,     TIFF_VARIABLE, TIFF_VARIABLE, TIFF_DOUBLE,	FIELD_CUSTOM, 1,	1,	(char *)"RawDataX" },
    { TIFFTAG_TESTPOINT_DATA_Y,     TIFF_VARIABLE, TIFF_VARIABLE, TIFF_DOUBLE,	FIELD_CUSTOM, 1,	1,	(char *)"RawDataY" }
};

static void registerCustomTIFFTags(TIFF *tif)
{
    /* Install the extended Tag field info */
    int error = TIFFMergeFieldInfo(tif, xtiffFieldInfo, TIFFFieldInfo_LEN(xtiffFieldInfo));
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
    const qint64 readBytes = device->read(static_cast<char *>(buf), size);
    return device->isReadable() ? static_cast<tsize_t>(readBytes): -1;
}

tsize_t qtiffWriteProc(thandle_t fd, tdata_t buf, tsize_t size)
{
    QIODevice *device = static_cast<QIODevice *>(fd);
    const qint64 writtenBytes = device->write(static_cast<char *>(buf), size);
    return static_cast<tsize_t>(writtenBytes);
}

toff_t qtiffSeekProc(thandle_t fd, toff_t off, int whence)
{
    QIODevice *device = static_cast<QIODevice *>(fd);
    switch (whence) {
    case SEEK_SET:
        device->seek(static_cast<qint64>(off));
        break;
    case SEEK_CUR:
        device->seek(device->pos() + static_cast<qint64>(off));
        break;
    case SEEK_END:
        device->seek(device->size() + static_cast<qint64>(off));
        break;
    }

    return static_cast<toff_t>(device->pos());
}

int qtiffCloseProc(thandle_t /*fd*/)
{
    return 0;
}

toff_t qtiffSizeProc(thandle_t fd)
{
    QIODevice *device = static_cast<QIODevice *>(fd);
    return static_cast<toff_t>(device->size());
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

void ImageTiff::convert32BitOrder(void *buffer, int width)
{
    uint32 *target = reinterpret_cast<uint32 *>(buffer);
    for (int32 x=0; x<width; ++x) {
        uint32 p = target[x];
        // convert between ARGB and ABGR
        target[x] = (p & 0xff000000)
                    | ((p & 0x00ff0000) >> 16)
                    | (p & 0x0000ff00)
                    | ((p & 0x000000ff) << 16);
    }
}

bool ImageTiff::readPage(QImage &image)
{
//    int compression;
    QImage::Format format;
    QSize size;
    uint16 photometric;
    bool grayscale;

    // Get Image's header
    uint32 width;
    uint32 height;
    if (!TIFFGetField(m_tiff, TIFFTAG_IMAGEWIDTH, &width)
        || !TIFFGetField(m_tiff, TIFFTAG_IMAGELENGTH, &height)
        || !TIFFGetField(m_tiff, TIFFTAG_PHOTOMETRIC, &photometric)) {
        return false;
    }
    size = QSize(static_cast<int>(width), static_cast<int>(height));

    uint16 orientationTag;
    TIFFGetField(m_tiff, TIFFTAG_ORIENTATION, &orientationTag);

    // BitsPerSample defaults to 1 according to the TIFF spec.
    uint16 bitPerSample;
    if (!TIFFGetField(m_tiff, TIFFTAG_BITSPERSAMPLE, &bitPerSample))
        bitPerSample = 1;
    uint16 samplesPerPixel; // they may be e.g. grayscale with 2 samples per pixel
    if (!TIFFGetField(m_tiff, TIFFTAG_SAMPLESPERPIXEL, &samplesPerPixel))
        samplesPerPixel = 1;

    grayscale = photometric == PHOTOMETRIC_MINISBLACK || photometric == PHOTOMETRIC_MINISWHITE;

    if (grayscale && bitPerSample == 1 && samplesPerPixel == 1)
        format = QImage::Format_Mono;
//    else if (photometric == PHOTOMETRIC_MINISBLACK && bitPerSample == 8 && samplesPerPixel == 1)
//        format = QImage::Format_Grayscale8;
    else if ((grayscale || photometric == PHOTOMETRIC_PALETTE) && bitPerSample == 8 && samplesPerPixel == 1)
        format = QImage::Format_Indexed8;
    else if (samplesPerPixel < 4)
        format = QImage::Format_RGB32;
    else {
        uint16 count;
        uint16 *extrasamples;
        // If there is any definition of the alpha-channel, libtiff will return premultiplied
        // data to us. If there is none, libtiff will not touch it and  we assume it to be
        // non-premultiplied, matching behavior of tested image editors, and how older Qt
        // versions used to save it.
        bool gotField = TIFFGetField(m_tiff, TIFFTAG_EXTRASAMPLES, &count, &extrasamples);
        if (!gotField || !count || extrasamples[0] == EXTRASAMPLE_UNSPECIFIED)
            format = QImage::Format_ARGB32;
        else
            format = QImage::Format_ARGB32_Premultiplied;
    }

    image = QImage(size, format);
    if (format == QImage::Format_Mono) {
        QVector<QRgb> colortable(2);
        if (photometric == PHOTOMETRIC_MINISBLACK) {
            colortable[0] = 0xff000000;
            colortable[1] = 0xffffffff;
        } else {
            colortable[0] = 0xffffffff;
            colortable[1] = 0xff000000;
        }
        image.setColorTable(colortable);

        if (!image.isNull()) {
            for (uint32 y=0; y<height; ++y) {
                if (TIFFReadScanline(m_tiff, image.scanLine(static_cast<int>(y)), y, 0) < 0) {
                    return false;
                }
            }
        }
    } else {
        if (format == QImage::Format_Indexed8) {
            if (!image.isNull()) {
                const uint16 tableSize = 256;
                QVector<QRgb> qtColorTable(tableSize);
                if (grayscale) {
                    for (int i = 0; i<tableSize; ++i) {
                        const int c = (photometric == PHOTOMETRIC_MINISBLACK) ? i : (255 - i);
                        qtColorTable[i] = qRgb(c, c, c);
                    }
                } else {
                    // create the color table
                    uint16 *redTable = nullptr;
                    uint16 *greenTable = nullptr;
                    uint16 *blueTable = nullptr;
                    if (!TIFFGetField(m_tiff, TIFFTAG_COLORMAP, &redTable, &greenTable, &blueTable)) {
                        return false;
                    }
                    if (!redTable || !greenTable || !blueTable) {
                        return false;
                    }

                    for (int i = 0; i<tableSize ;++i) {
                        const int red = redTable[i] / 257;
                        const int green = greenTable[i] / 257;
                        const int blue = blueTable[i] / 257;
                        qtColorTable[i] = qRgb(red, green, blue);
                    }
                }

                image.setColorTable(qtColorTable);
                for (uint32 y=0; y<height; ++y) {
                    if (TIFFReadScanline(m_tiff, image.scanLine(static_cast<int>(y)), y, 0) < 0) {
                        return false;
                    }
                }

                // free redTable, greenTable and greenTable done by libtiff
            }
//        } else if (format == QImage::Format_Grayscale8) {
//            if (!image.isNull()) {
//                for (uint32 y = 0; y < height; ++y) {
//                    if (TIFFReadScanline(m_tiff, image.scanLine(y), y, 0) < 0) {
//                        return false;
//                    }
//                }
//            }
        } else {
            if (!image.isNull()) {
                const int stopOnError = 1;
                if (TIFFReadRGBAImageOriented(m_tiff, width, height, reinterpret_cast<uint32 *>(image.bits()), orientationTag, stopOnError)) {
                    for (uint32 y=0; y<height; ++y)
                        convert32BitOrder(image.scanLine(static_cast<int>(y)), static_cast<int>(width));
                } else {
                    return false;
                }
            }
        }
    }

    if (image.isNull()) {
         return false;
    }

    float resX = 0;
    float resY = 0;
    uint16 resUnit;
    if (!TIFFGetField(m_tiff, TIFFTAG_RESOLUTIONUNIT, &resUnit))
        resUnit = RESUNIT_INCH;

    if (TIFFGetField(m_tiff, TIFFTAG_XRESOLUTION, &resX)
        && TIFFGetField(m_tiff, TIFFTAG_YRESOLUTION, &resY)) {

        switch(resUnit) {
        case RESUNIT_CENTIMETER:
            image.setDotsPerMeterX(qRound(resX * 100));
            image.setDotsPerMeterY(qRound(resY * 100));
            break;
        case RESUNIT_INCH:
            image.setDotsPerMeterX(qRound(resX * (100 / 2.54f)));
            image.setDotsPerMeterY(qRound(resY * (100 / 2.54f)));
            break;
        default:
            // do nothing as defaults have already
            // been set within the QImage class
            break;
        }
    }

    return true;
}

bool ImageTiff::decodeTestpointData(QString fileformat, TestpointMeasure &testpoint)
{
    Q_UNUSED(fileformat)
    char *rawDescription;
    if (!TIFFGetField(m_tiff, TIFFTAG_TESTPOINT_DESCRIPTION, &rawDescription)) {
        qWarning() << "Couldn't obtain testpoint's description";
        return false;
    }
    // Example: "POINT:10, X:232, Y:6522, SIG:sine, FREQ:1000, VOLT:2.6, SAMPLES:1000"
    QString description = QString::fromLatin1(rawDescription);
    QStringList args = description.split(", ");
    int id = -1;
    int x = -1;
    int y = -1;
    ToneWaveForm type;
    int freq = -1;
    qreal volt = -1.;
    int samplesCount = -1;
    bool ok = false;
    for (const QString &arg : args) {
        QString key = arg.section(":", 0, 0);
        QString value = arg.section(":", 1, 1);
        if (key == "POINT") {
            id = value.toInt(&ok);
            if (!ok) {
                qWarning() << "Invalid format of testpoint's descrtiption:" << description;
                return false;
            }
            continue;
        }
        if (key == "X") {
            x = value.toInt(&ok);
            if (!ok) {
                qWarning() << "Invalid format of testpoint's descrtiption:" << description;
                return false;
            }
            continue;
        }
        if (key == "Y") {
            y = value.toInt(&ok);
            if (!ok) {
                qWarning() << "Invalid format of testpoint's descrtiption:" << description;
                return false;
            }
            continue;
        }
        if (key == "SIG") {
            type = ToneWaveForm(value);
            if (type.id() == ToneWaveForm::WAVE_UNKNOWN) {
                qWarning() << "Invalid format of testpoint's descrtiption:" << description;
                return false;
            }
            continue;
        }
        if (key == "FREQ") {
            freq = value.toInt(&ok);
            if (!ok) {
                qWarning() << "Invalid format of testpoint's descrtiption:" << description;
                return false;
            }
            continue;
        }
        if (key == "VOLT") {
            volt = value.toDouble(&ok);
            if (!ok) {
                qWarning() << "Invalid format of testpoint's descrtiption:" << description;
                return false;
            }
            continue;
        }
        if (key == "SAMPLES") {
            samplesCount = value.toInt(&ok);
            if (!ok) {
                qWarning() << "Invalid format of testpoint's descrtiption:" << description;
                return false;
            }
            continue;
        }
    }
    if ((id < 0) || (x < 0) || (y < 0) || (type.id() == ToneWaveForm::WAVE_UNKNOWN) || (freq < 0) || (volt < 0.) || (samplesCount < 0)) {
        qWarning() << "Some fields is missed in testpoint's description:" << description;
        qWarning() << id << x << y << type << freq << volt << samplesCount;
        return false;
    }

    QList<QPointF> samples;
    if (samplesCount > 0) {
        uint16 count = 0;
        double *dataX = nullptr;
        double *dataY = nullptr;
        TIFFGetField(m_tiff, TIFFTAG_TESTPOINT_DATA_X, &count, &dataX);
        TIFFGetField(m_tiff, TIFFTAG_TESTPOINT_DATA_Y, &count, &dataY);
        for (int t = 0; t < samplesCount; t++) {
            QPointF point(dataX[t], dataY[t]);
            samples.append(point);
        }
    }

    QImage signatureImage;
    if (!readPage(signatureImage)) {
        qWarning() << "Can't read signature image";
        return false;
    }

    testpoint.id = id;
    testpoint.pos = QPoint(x, y);
    testpoint.signalType = type;
    testpoint.signalFrequency = freq;
    testpoint.signalVoltage = volt;
    testpoint.isCurrent = false;
    testpoint.signature = signatureImage;
    testpoint.data = samples;

    return true;
}

bool ImageTiff::readImageSeries(QString filePath, QImage &boardPhoto, QList<TestpointMeasure> &testpoints)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "File" << filePath << "couldn't be open for reading";
        Q_ASSERT(false);
        return false;
    }

    // Register custom tags
    augment_libtiff_with_custom_tags();

    // current implementation uses TIFFClientOpen which needs to be
    // able to seek, so sequential devices are not supported
    QByteArray header = file.peek(4);
    bool isTiffFile = (header == QByteArray::fromRawData("\x49\x49\x2A\x00", 4))
                      || (header == QByteArray::fromRawData("\x4D\x4D\x00\x2A", 4));
    if (!isTiffFile) {
        qWarning() << "File" << filePath << "is not a Tiff file";
        Q_ASSERT(false);
        return false;
    }

    m_tiff = TIFFClientOpen("foo", "r", &file,
                              qtiffReadProc, qtiffWriteProc, qtiffSeekProc,
                              qtiffCloseProc, qtiffSizeProc, qtiffMapProc,
                              qtiffUnmapProc);

    if (!m_tiff) {
        qWarning() << "Tiff library couldn't open file" << filePath;
        Q_ASSERT(false);
        return false;
    }

    unsigned int totalPages = 0;
    unsigned int page = 0;
    int fileType = 0;
    bool typeRead = TIFFGetField(m_tiff, TIFFTAG_SUBFILETYPE, &fileType);
    if (!typeRead || (fileType != FILETYPE_PAGE)) {
        qWarning() << "Tag" << TIFFTAG_SUBFILETYPE << "is missed or have unsupported type"
                   << fileType << "in file" << filePath;
        // Treat entire file as single photo - use it as board's photo
        if (!TIFFSetDirectory(m_tiff, 0)) {
            qWarning() << "Couldn't select first page in file" << filePath;
            goto error;
        }
        if (!readPage(boardPhoto)) {
            qWarning() << "Can't read board phoro from file:" << filePath;
            goto error;
        }
        goto success;
    }
    if (!TIFFGetField(m_tiff, TIFFTAG_PAGENUMBER, &page, &totalPages)) {
        qWarning() << "Couldn't obtain tag" << TIFFTAG_PAGENUMBER << "from file" << filePath;
        goto error;
    }
    qDebug() << "File" << filePath << "have" << totalPages << "pages";
    if (totalPages == 0) {
        qWarning() << "File doesn't have any page:" << filePath;
        goto error;
    }

    for (page = 0; page < totalPages; page++) {
        if (!TIFFSetDirectory(m_tiff, static_cast<uint16>(page))) {
            qWarning() << "Couldn't select page" << page << "for file" << filePath;
            goto error;
        }

        char *rawFileformat;
        if (!TIFFGetField(m_tiff, TIFFTAG_FILEFORMAT, &rawFileformat)) {
            if (page == 0) {
                qDebug() << "Read board's photo from page" << page;
                if (!readPage(boardPhoto)) {
                    qWarning() << "Can't read board phoro from file:" << filePath;
                    goto error;
                }
                continue;
            } else if (page == 1) {
                qDebug() << "Skip board's photo with testpoints location from page" << page;
                continue;
            } else {
                qWarning() << "Couldn't obtain testpoint's format at" << page << "from file" << filePath;
                goto error;
            }
        }
        QString fileformat = QString::fromLatin1(rawFileformat);
        qDebug() << "Testpoint at" << page << "have format:" << fileformat;

        TestpointMeasure measure;
        if (!decodeTestpointData(fileformat, measure)) {
            qWarning() << "Can't decode signature data from file:"
                       << filePath << "at page" << page;
            goto error;
        }
        testpoints.append(measure);
    }

success:
    qDebug() << "File" << filePath << "has been successfully read";
    TIFFClose(m_tiff);
    return true;

error:
    TIFFClose(m_tiff);
    Q_ASSERT(false);
    return false;
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
    if (!tiff) {
        qWarning() << "Tiff library couldn't open file" << filePath;
        return false;
    }

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
    TIFFSetField(tiff, TIFFTAG_TESTPOINT_DESCRIPTION, "POINT:10, X:232, Y:6522, SIG:sine, FREQ:1000, VOLT:2.6, SAMPLES:1000");

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

bool ImageTiff::writeImageSeries(QString filePath, const QImage &boardPhoto,
                                 const QImage &boardPhotoWithMarkers, const QList<TestpointMeasure> &testpoints)
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

    int totalPages = testpoints.count();
    if (!boardPhoto.isNull()) {
        totalPages++;
    }
    if (!boardPhotoWithMarkers.isNull()) {
        totalPages++;
    }

    int page = 0;
    if (!boardPhoto.isNull()) {
        TIFFSetField(m_tiff, TIFFTAG_SUBFILETYPE, FILETYPE_PAGE);
        TIFFSetField(m_tiff, TIFFTAG_PAGENUMBER, page, totalPages);
        if (!appendImage(boardPhoto)) {
            qWarning() << "Image" << boardPhoto << "couldn't be written in" << filePath;
            TIFFClose(m_tiff);
            Q_ASSERT(false);
            return false;
        }
        TIFFWriteDirectory(m_tiff);
        page++;
    }

    if (!boardPhotoWithMarkers.isNull()) {
        TIFFSetField(m_tiff, TIFFTAG_SUBFILETYPE, FILETYPE_PAGE);
        TIFFSetField(m_tiff, TIFFTAG_PAGENUMBER, page, totalPages);
        if (!appendImage(boardPhotoWithMarkers)) {
            qWarning() << "Image" << boardPhotoWithMarkers << "couldn't be written in" << filePath;
            TIFFClose(m_tiff);
            Q_ASSERT(false);
            return false;
        }
        TIFFWriteDirectory(m_tiff);
        page++;
    }

    for (int i = 0; i < testpoints.count(); i++) {
        const TestpointMeasure &testpoint = testpoints.at(i);
        const QImage &image = testpoint.signature;

        TIFFSetField(m_tiff, TIFFTAG_SUBFILETYPE, FILETYPE_PAGE);
        TIFFSetField(m_tiff, TIFFTAG_PAGENUMBER, page, totalPages);

        TIFFSetField(m_tiff, TIFFTAG_FILEFORMAT, "V1.0");
        // Example: "POINT:10, X:232, Y:6522, SIG:sine, FREQ:1000, VOLT:2.6, SAMPLES:1000"
        QString form = testpoint.signalType.getName();
        int samplesCount = testpoint.data.count();
        QString description = QString("POINT:%1, X:%2, Y:%3, SIG:%4, FREQ:%5, VOLT:%6, SAMPLES:%7")
                                     .arg(QString::number(testpoint.id))
                                     .arg(QString::number(testpoint.pos.x()))
                                     .arg(QString::number(testpoint.pos.y()))
                                     .arg(form)
                                     .arg(QString::number(testpoint.signalFrequency))
                                     .arg(QString::number(testpoint.signalVoltage, 'f', 1))
                                     .arg(QString::number(samplesCount));
        TIFFSetField(m_tiff, TIFFTAG_TESTPOINT_DESCRIPTION, description.toLatin1().data());

        if (samplesCount > 0) {
            double *dataX = (double *)malloc(samplesCount * sizeof(double));
            double *dataY = (double *)malloc(samplesCount * sizeof(double));
            for (int t = 0; t < samplesCount; t++) {
                QPointF point = testpoint.data.at(t);
                dataX[t] = point.x();
                dataY[t] = point.y();
            }
            TIFFSetField(m_tiff, TIFFTAG_TESTPOINT_DATA_X, samplesCount, dataX);
            TIFFSetField(m_tiff, TIFFTAG_TESTPOINT_DATA_Y, samplesCount, dataY);
            free(dataX);
            free(dataY);
        }

        if (!appendImage(image)) {
            qWarning() << "Image" << image << "couldn't be written in" << filePath;
            TIFFClose(m_tiff);
            Q_ASSERT(false);
            return false;
        }
        TIFFWriteDirectory(m_tiff);
        page++;
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
                        && TIFFSetField(m_tiff, TIFFTAG_XRESOLUTION, static_cast<double>(image.logicalDpiX()))
                        && TIFFSetField(m_tiff, TIFFTAG_YRESOLUTION, static_cast<double>(image.logicalDpiY()));
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
