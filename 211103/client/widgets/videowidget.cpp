#include "videowidget.h"

#include <QPainter>

VideoWidget::VideoWidget(QWidget *parent)
    : QWidget(parent)
{
    QPalette palette = this->palette();
    palette.setColor(QPalette::Background, Qt::black);
    this->setPalette(palette);
    this->setAutoFillBackground(true);
}

void VideoWidget::paintEvent(QPaintEvent*)
{
    if(image.isNull()) return;

    QPainter painter;
    painter.begin(this);

    QSize size = image.size().scaled(this->size(), Qt::KeepAspectRatio);
    QRectF rect({0, 0}, size);
    rect.moveCenter(this->rect().center());

    painter.drawImage(rect, image);

    painter.end();
}
