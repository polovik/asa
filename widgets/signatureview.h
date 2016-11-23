#ifndef SIGNATUREVIEW_H
#define SIGNATUREVIEW_H

#include <QObject>
#include <qcustomplot/qcustomplot.h>

typedef struct {
    QString type;
    double voltage;
    int frequency;
} SignalParameters;

class SignatureView : public QCustomPlot
{
    Q_OBJECT
public:
    SignatureView(QWidget *parent = 0);
    ~SignatureView();
    void setMaximumAmplitude(qreal voltage);
    void loadPreviousSignature(const QList<QPointF> &graphData);
    void draw(const QVector<double> &keys, const QVector<double> &values);
    void getView(SignalParameters params, QImage &renderedView, QList<QPointF> &graphData);
    
public slots:
    void saveView();
    
private:
    QCPItemPixmap *m_previousSignature;
    QCPGraph *m_graphPrevSignature;
    QCPGraph *m_graphCurSignature;
};

#endif // SIGNATUREVIEW_H
