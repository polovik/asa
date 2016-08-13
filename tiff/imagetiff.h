#ifndef IMAGETIFF_H
#define IMAGETIFF_H

#include <QObject>
#include "../formdiagnose.h"

typedef struct tiff TIFF;

class ImageTiff : public QObject
{
    Q_OBJECT
public:
    explicit ImageTiff(QObject *parent = 0);
    ~ImageTiff();

    bool readImageSeries(QString filePath, QImage &boardPhoto, QList<TestpointMeasure> &testpoints);

    bool write(QString filePath, const QImage &image);
    bool writeImageSeries(QString filePath, const QImage &boardPhoto,
                          const QImage &boardPhotoWithMarkers, const QList<TestpointMeasure> &testpoints);

    enum Compression {
        NoCompression = 0,
        LzwCompression = 1
    };

public slots:

signals:

private:
    void convert32BitOrder(void *buffer, int width);
    bool readPage(QImage &image);
    bool appendImage(const QImage &image);

    TIFF *m_tiff;
};

#endif // IMAGETIFF_H
