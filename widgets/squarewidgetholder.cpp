#include <QDebug>
#include <QResizeEvent>
#include "squarewidgetholder.h"

SquareWidgetHolder::SquareWidgetHolder(QWidget *parent) : QWidget(parent)
{

}

SquareWidgetHolder::~SquareWidgetHolder()
{

}

void SquareWidgetHolder::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    QSize newSize = event->size();
    
    QList<QWidget *>widgets = findChildren<QWidget *>();
    if (widgets.count() > 1) {
        qWarning() << "SquareWidgetHolder may hold only one widget, but holds now:" << widgets.count();
        return;
    }
    
    foreach(QWidget *child, widgets) {
        int width = 20;
        int height = 20;
        if (newSize.width() > newSize.height()) {
            width = newSize.height();
        } else {
            width = newSize.width();
        }
        height = width;
        int x = (newSize.width() - width) / 2;
        int y = (newSize.height() - height) / 2;
        child->setGeometry(x, y, width, height);
    }
}

