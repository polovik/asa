#include "oscilloscopeview.h"
#include "qcustomplot/qcustomplot.h"

OscilloscopeView::OscilloscopeView(QWidget *parent) : QCustomPlot(parent)
{
    m_movedStraightLine = nullptr;
    
    m_graphChannelLeft = addGraph();
    m_graphChannelRight = addGraph();
    m_graphChannelLeft->setPen(QColor("darkBlue"));
    m_graphChannelRight->setPen(QColor("darkGreen"));
    xAxis->setLabel(tr("Milliseconds"));
    yAxis->setLabel(tr("Volts"));
    xAxis->setRange(0.0, 1024. * 0.016);
    yAxis->setRange(0.0, 50.0);
    QVector<double> dataKeys(2);
    QVector<double> dataLeft(2), dataRight(2);
    dataKeys[0] = -1.;
    dataKeys[1] = 1.;
    dataLeft[0] = dataRight[1] = -1.;
    dataLeft[1] = dataRight[0] = 1.;
    m_graphChannelLeft->setData(dataKeys, dataLeft);
    m_graphChannelRight->setData(dataKeys, dataRight);
    QPen zeroLinePen;
    zeroLinePen.setColor("lightsalmon");
    zeroLinePen.setStyle(Qt::SolidLine);
    zeroLinePen.setWidth(2);
    xAxis->grid()->setZeroLinePen(zeroLinePen);
    yAxis->grid()->setZeroLinePen(zeroLinePen);
    
    setInteraction(QCP::iRangeZoom, true);
    xAxis->axisRect()->setRangeZoom(Qt::Horizontal);
    yAxis->axisRect()->setRangeZoom(Qt::Horizontal);
    setInteraction(QCP::iRangeDrag, true);
    xAxis->axisRect()->setRangeDrag(Qt::Horizontal);
    yAxis->axisRect()->setRangeDrag(Qt::Horizontal);
    setInteraction(QCP::iSelectItems, true);
    setInteraction(QCP::iMultiSelect, false);
//    graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssTriangle, 10));

    //  Display value under mouse cursor
    m_pointUnderMouse = new QCPItemTracer(this);
    addItem(m_pointUnderMouse);
    m_pointUnderMouse->setGraph(graph(0));
    m_pointUnderMouse->setGraphKey(0.0);
    m_pointUnderMouse->setInterpolating(true);
    m_pointUnderMouse->setStyle(QCPItemTracer::tsCircle);
    m_pointUnderMouse->setPen(QPen(Qt::red));
    m_pointUnderMouse->setBrush(Qt::red);
    m_pointUnderMouse->setSize(7);
    
    m_pointUnderMouseText = new QCPItemText(this);
    addItem(m_pointUnderMouseText);
    m_pointUnderMouseText->position->setType(QCPItemPosition::ptPlotCoords);
    m_pointUnderMouseText->setPositionAlignment(Qt::AlignLeft | Qt::AlignTop);
    m_pointUnderMouseText->setTextAlignment(Qt::AlignLeft);
    m_pointUnderMouseText->setFont(QFont(font().family(), 9));
    m_pointUnderMouseText->setPadding(QMargins(8, 0, 0, 0));
    QCPData pointData = graph(0)->data()->lowerBound(0.0).value();
    m_pointUnderMouseText->position->setCoords(pointData.key, pointData.value);
    m_pointUnderMouseText->setText(tr("%1ms, %2V").arg(pointData.key, 0, 'f', 2).arg(pointData.value, 0, 'f', 1));
//    m_pointUnderMouseText->setBrush(Qt::white);
    
    //  Measure time between two points
    QPen measurementPen;
    measurementPen.setColor("brown");
    measurementPen.setStyle(Qt::DotLine);
    m_timeMeasurementLine1 = new QCPItemStraightLine(this);
    addItem(m_timeMeasurementLine1);
    m_timeMeasurementLine1->setPen(measurementPen);
    m_timeMeasurementLine1->point1->setCoords(1.0, 0.0);
    m_timeMeasurementLine1->point2->setCoords(1.0, 1.0);
    
    m_timeMeasurementLine2 = new QCPItemStraightLine(this);
    addItem(m_timeMeasurementLine2);
    m_timeMeasurementLine2->setPen(measurementPen);
    m_timeMeasurementLine2->point1->setCoords(10.0, 0.0);
    m_timeMeasurementLine2->point2->setCoords(10.0, 1.0);
    
    // add the bracket at the top:
    m_timeMeasurementBracket = new QCPItemBracket(this);
    addItem(m_timeMeasurementBracket);
    m_timeMeasurementBracket->setPen(measurementPen);
    m_timeMeasurementBracket->left->setCoords(1.0, 48.0);
    m_timeMeasurementBracket->right->setCoords(10.0, 48.0);
    m_timeMeasurementBracket->setLength(10);
    m_timeMeasurementBracket->setStyle(QCPItemBracket::bsSquare);
    
    m_timeMeasurementText = new QCPItemText(this);
    addItem(m_timeMeasurementText);
    m_timeMeasurementText->position->setParentAnchor(m_timeMeasurementBracket->center);
    m_timeMeasurementText->position->setCoords(0, 0);
    m_timeMeasurementText->setPositionAlignment(Qt::AlignTop | Qt::AlignHCenter);
    double mes1 = m_timeMeasurementLine1->point1->key();
    double mes2 = m_timeMeasurementLine2->point2->key();
    QString measuredTime = QString::number(qAbs(mes1 - mes2), 'f', 2) + tr(" ms");
    m_timeMeasurementText->setText(measuredTime);
    m_timeMeasurementText->setFont(QFont(font().family(), 10));
    m_timeMeasurementText->setBrush(Qt::white);
    
    // Measure time between two points
    m_voltageMeasurementLine1 = new QCPItemStraightLine(this);
    addItem(m_voltageMeasurementLine1);
    m_voltageMeasurementLine1->setPen(measurementPen);
    m_voltageMeasurementLine1->point1->setCoords(-100.0, 1.0);
    m_voltageMeasurementLine1->point2->setCoords(100.0,  1.0);

    m_voltageMeasurementLine2 = new QCPItemStraightLine(this);
    addItem(m_voltageMeasurementLine2);
    m_voltageMeasurementLine2->setPen(measurementPen);
    m_voltageMeasurementLine2->point1->setCoords(-100.0, -1.0);
    m_voltageMeasurementLine2->point2->setCoords(100.0,  -1.0);

    m_voltageMeasurementText1 = new QCPItemText(this);
    addItem(m_voltageMeasurementText1);
    m_voltageMeasurementText1->position->setType(QCPItemPosition::ptPlotCoords);
    m_voltageMeasurementText1->position->setCoords(0, 1.0);
    m_voltageMeasurementText1->setPositionAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
    mes1 = m_voltageMeasurementLine1->point1->value();
    m_voltageMeasurementText1->setText(QString::number(mes1, 'f', 2));
    m_voltageMeasurementText1->setFont(QFont(font().family(), 10));
    m_voltageMeasurementText1->setBrush(Qt::white);

    m_voltageMeasurementText2 = new QCPItemText(this);
    addItem(m_voltageMeasurementText2);
    m_voltageMeasurementText2->position->setType(QCPItemPosition::ptPlotCoords);
    m_voltageMeasurementText2->position->setCoords(0, -1.0);
    m_voltageMeasurementText2->setPositionAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
    mes2 = m_voltageMeasurementLine2->point1->value();
    m_voltageMeasurementText2->setText(QString::number(mes2, 'f', 2));
    m_voltageMeasurementText2->setFont(QFont(font().family(), 10));
    m_voltageMeasurementText2->setBrush(Qt::white);

    // Adjust trigger level
    QPen triggerPen;
    triggerPen.setWidth(2);
    triggerPen.setColor("green");
    triggerPen.setStyle(Qt::DashLine);
    m_triggerLevelLine = new QCPItemStraightLine(this);
    addItem(m_triggerLevelLine);
    m_triggerLevelLine->setPen(triggerPen);
    m_triggerLevelLine->point1->setCoords(-100.0, 1.0);
    m_triggerLevelLine->point2->setCoords(100.0, 1.0);
    
    m_triggerLevelText = new QCPItemText(this);
    addItem(m_triggerLevelText);
    m_triggerLevelText->setPositionAlignment(Qt::AlignRight | Qt::AlignVCenter);
    m_triggerLevelText->position->setParentAnchor(m_triggerLevelLine->anchor("point2"));
    m_triggerLevelText->position->setCoords(0, 0);
    double triggerVoltage = m_triggerLevelLine->point1->value();
    QString voltage = QString::number(triggerVoltage, 'f', 3) + tr(" V");
    m_triggerLevelText->setText(voltage);
    m_triggerLevelText->setFont(QFont(font().family(), 10));
    m_triggerLevelText->setBrush(Qt::white);
    m_triggerLevelText->setSelectable(false);

    showTimeMeasureGuides(true);
    showVoltageMeasureGuides(false, true);
    showSampleValueUnderMouse(true);
}

OscilloscopeView::~OscilloscopeView()
{

}

void OscilloscopeView::draw(DisplayedGraphId id, const QVector<double> &keys, const QVector<double> &values)
{
    if (id == GRAPH_CHANNEL_LEFT)
        m_graphChannelLeft->setData(keys, values);
    if (id == GRAPH_CHANNEL_RIGHT)
        m_graphChannelRight->setData(keys, values);
    replot();
}

void OscilloscopeView::showPointToolTip(QMouseEvent *event)
{
    double plotX = xAxis->pixelToCoord(event->pos().x());
    double plotY = yAxis->pixelToCoord(event->pos().y());
    
    QCPData pointData = graph(0)->data()->lowerBound(plotX).value();
    
    double time = pointData.key;
    double voltage = pointData.value;
    
    double closeness = qAbs(voltage - plotY);
    double percent = closeness / yAxis->range().size();
    
//    qDebug() << "Current point is: Time:" << plotX << time << "Voltage:" << plotY << voltage << closeness << percent;

    if (graph(0)->data()->contains(time) == false)
        return;
        
    if (percent > 0.1)
        return;
        
    m_pointUnderMouse->setGraphKey(time);
    
    replot();
    
    m_pointUnderMouseText->position->setCoords(time, voltage);
    m_pointUnderMouseText->setText(tr("%1ms, %2V").arg(time, 0, 'f', 2).arg(voltage, 0, 'f', 1));
}

void OscilloscopeView::saveView()
{
    QPixmap view = toPixmap();
    
    QDir currentDir = QDir::current();
    if (!currentDir.exists("oscilloscope")) {
        if (!currentDir.mkdir("oscilloscope")) {
            qWarning() << "Can't create folder \"oscilloscope\" in" << currentDir.absolutePath();
            return;
        }
    }
    if (!currentDir.cd("oscilloscope")) {
        qWarning() << "Can't enter in folder \"oscilloscope\" from" << currentDir.absolutePath();
        return;
    }
    
    QDateTime currentDate = QDateTime::currentDateTime();
    QString dateTime = currentDate.toString("hh_mm_ss dd_MM_yyyy");
    QString fileName = currentDir.absolutePath() + QDir::separator() + dateTime + ".png";
    
    if (!view.save(fileName)) {
        qWarning() << "Can't save Oscilloscope view to" << fileName;
        return;
    }
    
    qDebug() << "oscilloscope's view has been saved to" << fileName;
}

void OscilloscopeView::setXaxisRange(double minValue, double maxValue)
{
    m_axisXminValue = minValue;
    m_axisXmaxValue = maxValue;
    xAxis->setRange(m_axisXminValue, m_axisXmaxValue);
    yAxis->setRange(m_axisYminValue, m_axisYmaxValue);
    
    m_triggerLevelLine->point1->setCoords(-100., m_triggerLevelLine->point1->value());
    m_triggerLevelLine->point2->setCoords(m_axisXmaxValue, m_triggerLevelLine->point2->value());
    
    replot();
}

void OscilloscopeView::setYaxisRange(double minValue, double maxValue)
{
    m_axisYminValue = minValue;
    m_axisYmaxValue = maxValue;
    yAxis->setRange(m_axisYminValue, m_axisYmaxValue);
    xAxis->setRange(m_axisXminValue, m_axisXmaxValue);
    
    if (m_triggerLevelLine->point1->value() < m_axisYminValue) {
        setTriggerLevel(m_axisYminValue);
        QString triggerVoltage = QString::number(m_axisYminValue, 'f', 3) + tr(" V");
        m_triggerLevelText->setText(triggerVoltage);
    }
    if (m_triggerLevelLine->point1->value() > m_axisYmaxValue) {
        setTriggerLevel(m_axisYmaxValue);
        QString triggerVoltage = QString::number(m_axisYmaxValue, 'f', 3) + tr(" V");
        m_triggerLevelText->setText(triggerVoltage);
    }
    
    m_timeMeasurementBracket->left->setCoords(m_timeMeasurementBracket->left->key(), m_axisYmaxValue - 1.0);
    m_timeMeasurementBracket->right->setCoords(m_timeMeasurementBracket->right->key(), m_axisYmaxValue - 1.0);
    
    replot();
}

void OscilloscopeView::setTriggerLevel(double voltage)
{
    m_triggerLevelLine->point1->setCoords(m_triggerLevelLine->point1->key(), voltage);
    m_triggerLevelLine->point2->setCoords(m_triggerLevelLine->point2->key(), voltage);
}

void OscilloscopeView::showTriggerLine(bool visible)
{
    m_triggerLevelLine->setSelectable(visible);
    m_triggerLevelLine->setVisible(visible);
    m_triggerLevelText->setVisible(visible);
    replot();
}

void OscilloscopeView::showGraph(DisplayedGraphId id, bool visible)
{
    if (id == GRAPH_CHANNEL_LEFT)
        m_graphChannelLeft->setVisible(visible);
    if (id == GRAPH_CHANNEL_RIGHT)
        m_graphChannelRight->setVisible(visible);
    replot();
}

void OscilloscopeView::showTimeMeasureGuides(bool visible)
{
    m_timeMeasurementLine1->setVisible(visible);
    m_timeMeasurementLine1->setSelectable(visible);
    m_timeMeasurementLine2->setVisible(visible);
    m_timeMeasurementLine2->setSelectable(visible);
    m_timeMeasurementBracket->setVisible(visible);
    m_timeMeasurementBracket->setSelectable(false);
    m_timeMeasurementText->setVisible(visible);
    m_timeMeasurementText->setSelectable(false);
    replot();
}

void OscilloscopeView::showVoltageMeasureGuides(bool visible, bool freezed)
{
    m_voltageMeasurementLine1->setVisible(visible);
    m_voltageMeasurementLine1->setSelectable(!freezed);
    m_voltageMeasurementLine2->setVisible(visible);
    m_voltageMeasurementLine2->setSelectable(!freezed);
    m_voltageMeasurementText1->setVisible(visible);
    m_voltageMeasurementText1->setSelectable(false);
    m_voltageMeasurementText2->setVisible(visible);
    m_voltageMeasurementText2->setSelectable(false);
}

void OscilloscopeView::showSampleValueUnderMouse(bool visible)
{
    m_pointUnderMouse->setVisible(visible);
    m_pointUnderMouse->setSelectable(false);
    m_pointUnderMouseText->setVisible(visible);
    m_pointUnderMouseText->setSelectable(false);
}

void OscilloscopeView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        QCustomPlot::mousePressEvent(event);
        return;
    }

    QCPAbstractItem *selectedItem = itemAt(event->localPos(), true);
    if (selectedItem == nullptr) {
        QCustomPlot::mousePressEvent(event);
        return;
    }
    qDebug() << "Selected item:" << selectedItem;

    QCPItemStraightLine *straightLine = qobject_cast<QCPItemStraightLine *>(selectedItem);
    if (straightLine == nullptr) {
        QCustomPlot::mousePressEvent(event);
        return;
    }
    
    if (!selectedItems().empty()) {
        for (QCPAbstractItem *item : selectedItems()) {
            item->setSelected(false);
        }
    }
    
    m_movedStraightLine = straightLine;
    m_movedStraightLine->setSelected(true);
    
    double plotX = xAxis->pixelToCoord(event->pos().x());
    double plotY = yAxis->pixelToCoord(event->pos().y());
    QCPData pointData = graph(0)->data()->lowerBound(plotX).value();
    double time = pointData.key;
    double voltage = pointData.value;
    
    if (m_movedStraightLine == m_triggerLevelLine) {
        QString triggerVoltage = QString::number(plotY, 'f', 3) + tr(" V");
        m_triggerLevelText->setText(triggerVoltage);
        emit triggerLevelChanged(plotY);
    } else {
        m_pointUnderMouse->setGraphKey(time);
        m_pointUnderMouseText->position->setCoords(time, voltage);
        m_pointUnderMouseText->setText(tr("%1ms, %2V").arg(time, 0, 'f', 2).arg(voltage, 0, 'f', 1));
    }
    
    QWidget::mousePressEvent(event);
    replot();
}

void OscilloscopeView::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() != Qt::MiddleButton) {
        QCustomPlot::mouseDoubleClickEvent(event);
        return;
    }
    xAxis->setRange(m_axisXminValue, m_axisXmaxValue);
    yAxis->setRange(m_axisYminValue, m_axisYmaxValue);
    replot();
    event->accept();
}

void OscilloscopeView::mouseMoveEvent(QMouseEvent *event)
{
    if (m_movedStraightLine == nullptr) {
        QCustomPlot::mouseMoveEvent(event);
        showPointToolTip(event);
        return;
    }
    
    double plotX = xAxis->pixelToCoord(event->pos().x());
    double plotY = yAxis->pixelToCoord(event->pos().y());
    QCPData pointData = graph(0)->data()->lowerBound(plotX).value();
    double time = pointData.key;
    double voltage = pointData.value;
    
//    qDebug() << "move: time -" << plotX << time << "voltage -" << plotY << voltage;

    if (m_movedStraightLine == m_triggerLevelLine) {
        if (plotY < m_axisYminValue)
            plotY = m_axisYminValue;
        if (plotY > m_axisYmaxValue)
            plotY = m_axisYmaxValue;
        setTriggerLevel(plotY);
        QString triggerVoltage = QString::number(plotY, 'f', 3) + tr(" V");
        m_triggerLevelText->setText(triggerVoltage);
        emit triggerLevelChanged(plotY);
    } else {
        if (graph(0)->data()->contains(time) == false) {
            QWidget::mouseMoveEvent(event);
            return;
        }
        
        m_movedStraightLine->point1->setCoords(time, 0.0);
        m_movedStraightLine->point2->setCoords(time, 1.0);
        
        m_pointUnderMouse->setGraphKey(time);
        m_pointUnderMouseText->position->setCoords(time, voltage);
        m_pointUnderMouseText->setText(tr("%1ms, %2V").arg(time, 0, 'f', 2).arg(voltage, 0, 'f', 1));
        
        double mes1 = m_timeMeasurementLine1->point1->key();
        double mes2 = m_timeMeasurementLine2->point2->key();
        m_timeMeasurementBracket->left->setCoords(qMin(mes1, mes2), m_timeMeasurementBracket->left->value());
        m_timeMeasurementBracket->right->setCoords(qMax(mes1, mes2), m_timeMeasurementBracket->right->value());
        QString measuredTime = QString::number(qAbs(mes1 - mes2), 'f', 2) + tr(" ms");
        m_timeMeasurementText->setText(measuredTime);
    }
    
    QWidget::mouseMoveEvent(event);
    replot();
}

void OscilloscopeView::mouseReleaseEvent(QMouseEvent *event)
{
    if (!selectedItems().empty()) {
        for (QCPAbstractItem *item : selectedItems()) {
            item->setSelected(false);
        }
    }
    m_movedStraightLine = nullptr;
    
    QCustomPlot::mouseReleaseEvent(event);
    replot();
}

void OscilloscopeView::wheelEvent(QWheelEvent *event)
{
    Qt::KeyboardModifiers keys = event->modifiers();
    if ((keys & Qt::ShiftModifier) || (keys & Qt::ControlModifier) || (keys & Qt::AltModifier)) {
        xAxis->axisRect()->setRangeZoom(Qt::Horizontal | Qt::Vertical);
        yAxis->axisRect()->setRangeZoom(Qt::Horizontal | Qt::Vertical);
        xAxis->axisRect()->setRangeDrag(Qt::Horizontal | Qt::Vertical);
        yAxis->axisRect()->setRangeDrag(Qt::Horizontal | Qt::Vertical);
    }

    QCustomPlot::wheelEvent(event);

    xAxis->axisRect()->setRangeZoom(Qt::Horizontal);
    yAxis->axisRect()->setRangeZoom(Qt::Horizontal);
    xAxis->axisRect()->setRangeDrag(Qt::Horizontal);
    yAxis->axisRect()->setRangeDrag(Qt::Horizontal);
}
