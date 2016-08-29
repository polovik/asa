#include "signatureview.h"
#include "qcustomplot/qcustomplot.h"

SignatureView::SignatureView(QWidget *parent) : QCustomPlot(parent)
{
    m_graphPrevSignature = addGraph();
    m_graphCurSignature = addGraph();
    m_graphPrevSignature->setPen(QColor("black"));
    m_graphCurSignature->setPen(QColor("red"));
    xAxis->setLabel(tr("Ugenerator, Volts"));
    yAxis->setLabel(tr("Uprobe, Volts"));
    setMaximumAmplitude(30.);
    QPen zeroLinePen;
    zeroLinePen.setColor("lightsalmon");
    zeroLinePen.setStyle(Qt::SolidLine);
    zeroLinePen.setWidth(2);
    xAxis->grid()->setZeroLinePen(zeroLinePen);
    yAxis->grid()->setZeroLinePen(zeroLinePen);
}

SignatureView::~SignatureView()
{

}

void SignatureView::setMaximumAmplitude(qreal voltage)
{
    xAxis->setRange(-voltage * 1.1, voltage * 1.1);
    yAxis->setRange(-voltage * 1.1, voltage * 1.1);
    replot();
}

void SignatureView::loadPreviousSignature(const QList<QPointF> &graphData)
{
    QVector<double> keys;
    QVector<double> values;
    foreach(const QPointF &point, graphData) {
        keys.append(point.x());
        values.append(point.y());
    }
    m_graphPrevSignature->setData(keys, values);
    replot();
}

void SignatureView::draw(const QVector<double> &keys, const QVector<double> &values)
{
    m_graphCurSignature->setData(keys, values);
    replot();
}

void SignatureView::getView(QImage &renderedView, QList<QPointF> &graphData)
{
    m_graphPrevSignature->setVisible(false);
    replot(QCustomPlot::rpImmediate);
    QPixmap view = toPixmap();
    m_graphPrevSignature->setVisible(true);
    replot(QCustomPlot::rpImmediate);
    renderedView = view.toImage();
    
    QCPDataMap *rawData = m_graphCurSignature->data();
    graphData.clear();
    foreach(const QCPData &point, rawData->values()) {
        QPointF pt(point.key, point.value);
        graphData.append(pt);
    }
}

void SignatureView::saveView()
{
    m_graphPrevSignature->setVisible(false);
    replot(QCustomPlot::rpImmediate);
    QPixmap view = toPixmap();
    m_graphPrevSignature->setVisible(true);
    replot(QCustomPlot::rpImmediate);
    
    QDir currentDir = QDir::current();
    if (!currentDir.exists("signature")) {
        if (!currentDir.mkdir("signature")) {
            qWarning() << "Can't create folder \"signature\" in" << currentDir.absolutePath();
            return;
        }
    }
    if (!currentDir.cd("signature")) {
        qWarning() << "Can't enter in folder \"signature\" from" << currentDir.absolutePath();
        return;
    }
    
    QDateTime currentDate = QDateTime::currentDateTime();
    QString dateTime = currentDate.toString("hh_mm_ss dd_MM_yyyy");
    QString fileName = currentDir.absolutePath() + QDir::separator() + dateTime + ".png";
    
    if (!view.save(fileName)) {
        qWarning() << "Can't save Signature view to" << fileName;
        return;
    }
    
    qDebug() << "signature's view has been saved to" << fileName;
}

