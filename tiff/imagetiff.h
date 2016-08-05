#ifndef IMAGETIFF_H
#define IMAGETIFF_H

#include <QObject>

class ImageTiff : public QObject
{
    Q_OBJECT
public:
    explicit ImageTiff(QObject *parent = 0);
    ~ImageTiff();

    bool write(QString filePath, const QImage &image);

    enum Compression {
        NoCompression = 0,
        LzwCompression = 1
    };

public slots:

signals:

};

#endif // IMAGETIFF_H
