#include "signatureview.h"
#include "qcustomplot/qcustomplot.h"

SignatureView::SignatureView(QWidget *parent) : QCustomPlot(parent)
{
    m_graphPrevSignature = addGraph();
    m_graphPrevSignature->setPen(QColor("black"));
    m_graphPrevSignature->setLineStyle(QCPGraph::lsNone);
    QCPScatterStyle dotStylePrev(QCPScatterStyle::ssCircle, QColor("black"), QColor("black"), 2);
    m_graphPrevSignature->setScatterStyle(dotStylePrev);

    m_graphCurSignature = addGraph();
    m_graphCurSignature->setPen(QColor("red"));
    m_graphCurSignature->setLineStyle(QCPGraph::lsNone);
    QCPScatterStyle dotStyleCur(QCPScatterStyle::ssCircle, QColor("red"), QColor("red"), 2);
    m_graphCurSignature->setScatterStyle(dotStyleCur);

    xAxis->setLabel(tr("Ugenerator, Volts"));
    yAxis->setLabel(tr("Uprobe, Volts"));
    setMaximumAmplitude(30.);
    QPen zeroLinePen;
    zeroLinePen.setColor("lightsalmon");
    zeroLinePen.setStyle(Qt::SolidLine);
    zeroLinePen.setWidth(2);
    xAxis->grid()->setZeroLinePen(zeroLinePen);
    yAxis->grid()->setZeroLinePen(zeroLinePen);

    setPreviousSignatureVisible(false);
    setCurrentSignatureVisible(false);
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

void SignatureView::setPreviousSignatureVisible(bool show)
{
    m_showPrevSignature = show;
    m_graphPrevSignature->setVisible(m_showPrevSignature);
    replot();
}

void SignatureView::setCurrentSignatureVisible(bool show)
{
    m_showCurSignature = show;
    m_graphCurSignature->setVisible(m_showCurSignature);
    replot();
}

void SignatureView::loadPreviousSignature(const QList<QPointF> &graphData)
{
    QVector<double> keys;
    QVector<double> values;
    for (const QPointF &point : graphData) {
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

void SignatureView::getView(SelectedSignatures selects, SignalParameters params, QImage &renderedView, QList<QPointF> &graphData)
{
    // Add text labels about tested signal parameters
    qreal textOffset = 0.01;
    int fontSize = 9;
    QColor textColor(Qt::darkGreen);
    QCPItemText *signalType = new QCPItemText(this);
    addItem(signalType);
    signalType->setText(params.type);
    signalType->position->setType(QCPItemPosition::ptAxisRectRatio);
    signalType->setPositionAlignment(Qt::AlignLeft | Qt::AlignTop);
    signalType->position->setCoords(0.02, textOffset);
    signalType->setTextAlignment(Qt::AlignLeft);
    signalType->setFont(QFont(font().family(), fontSize));
    signalType->setColor(textColor);
    QCPItemText *signalVolt = new QCPItemText(this);
    addItem(signalVolt);
    signalVolt->setText(QString::number(params.voltage, 'f', 1) + tr("V"));
    signalVolt->position->setType(QCPItemPosition::ptAxisRectRatio);
    signalVolt->setPositionAlignment(Qt::AlignLeft | Qt::AlignTop);
    signalVolt->position->setParentAnchor(signalType->anchor("bottomLeft"));
    signalVolt->position->setCoords(0.00, textOffset);
    signalVolt->setTextAlignment(Qt::AlignLeft);
    signalVolt->setFont(QFont(font().family(), fontSize));
    signalVolt->setColor(textColor);
    QCPItemText *signalFreq = new QCPItemText(this);
    addItem(signalFreq);
    signalFreq->setText(QString::number(params.frequency) + tr("Hz"));
    signalFreq->position->setType(QCPItemPosition::ptAxisRectRatio);
    signalFreq->setPositionAlignment(Qt::AlignLeft | Qt::AlignTop);
    signalFreq->position->setParentAnchor(signalVolt->anchor("bottomLeft"));
    signalFreq->position->setCoords(0.00, textOffset);
    signalFreq->setTextAlignment(Qt::AlignLeft);
    signalFreq->setFont(QFont(font().family(), fontSize));
    signalFreq->setColor(textColor);

    // Render current view for store in image file.
    m_graphPrevSignature->setVisible(selects & PREVIOUS_SIGNATURE);
    m_graphCurSignature->setVisible(selects & CURRENT_SIGNATURE);
    replot(QCustomPlot::rpImmediate);
    QPixmap view = toPixmap();
    renderedView = view.toImage();

    // Remove temporaty text labels and unhide previous signature
    m_graphPrevSignature->setVisible(m_showPrevSignature);
    m_graphCurSignature->setVisible(m_showCurSignature);
    removeItem(signalType);
    removeItem(signalVolt);
    removeItem(signalFreq);
    replot(QCustomPlot::rpImmediate);
    
    // Get data of current signature
    QCPDataMap *rawData = m_graphCurSignature->data();
    graphData.clear();
    for (const QCPData &point : rawData->values()) {
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

