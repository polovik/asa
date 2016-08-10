#ifndef IMAGETIFF_H
#define IMAGETIFF_H

#include <QObject>

typedef struct tiff TIFF;

class ImageTiff : public QObject
{
    Q_OBJECT
public:
    explicit ImageTiff(QObject *parent = 0);
    ~ImageTiff();

    bool write(QString filePath, const QImage &image);
    bool writeImageSeries(QString filePath, QList<QImage> images);

    enum Compression {
        NoCompression = 0,
        LzwCompression = 1
    };

public slots:

signals:

private:
    bool appendImage(const QImage &image);

    TIFF *m_tiff;
};

#endif // IMAGETIFF_H
