#ifndef SIGNATUREVIEW_H
#define SIGNATUREVIEW_H

#include <QObject>
#include <qcustomplot/qcustomplot.h>

typedef enum {
    NOONE_SIGNATURE     = 0,
    PREVIOUS_SIGNATURE  = 1,
    CURRENT_SIGNATURE   = 2,
    ALL_SIGNATURES      = PREVIOUS_SIGNATURE | CURRENT_SIGNATURE,
} SelectedSignatures;

typedef struct {
    QString type;
    double voltage;
    int frequency;
} SignalParameters;

class SignatureView : public QCustomPlot
{
    Q_OBJECT
public:
    SignatureView(QWidget *parent = nullptr);
    ~SignatureView();
    void setMaximumAmplitude(qreal voltage);
    void setPreviousSignatureVisible(bool show);
    void setCurrentSignatureVisible(bool show);
    void loadPreviousSignature(const QList<QPointF> &graphData);
    void draw(const QVector<double> &keys, const QVector<double> &values);
    void getView(SelectedSignatures selects, SignalParameters params, QImage &renderedView, QList<QPointF> &graphData);
    
public slots:
    void saveView();
    
private:
    bool m_showPrevSignature;
    bool m_showCurSignature;
    QCPGraph *m_graphPrevSignature;
    QCPGraph *m_graphCurSignature;
};

#endif // SIGNATUREVIEW_H
