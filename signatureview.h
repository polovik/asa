#ifndef SIGNATUREVIEW_H
#define SIGNATUREVIEW_H

#include <QObject>
#include <qcustomplot/qcustomplot.h>

class SignatureView : public QCustomPlot
{
    Q_OBJECT
public:
    SignatureView(QWidget *parent = 0);
    ~SignatureView();
    void loadPreviousSignature(QList<QPointF> &graphData);
    void draw(const QVector<double> &keys, const QVector<double> &values);
    void getView(QImage &renderedView, QList<QPointF> &graphData);

public slots:
    void saveView();

private:
    QCPItemPixmap *m_previousSignature;
    QCPGraph *m_graphPrevSignature;
    QCPGraph *m_graphCurSignature;
};

#endif // SIGNATUREVIEW_H
