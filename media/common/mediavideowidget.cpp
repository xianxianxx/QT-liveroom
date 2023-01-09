#include "mediavideowidget.h"

#include <QPainter>
#include <QDebug>

MediaVideoWidget::MediaVideoWidget(QWidget *parent)
    : QWidget(parent)
{
    setAutoFillBackground(true);
    QPalette palette = this->palette();
    palette.setColor(QPalette::Window, Qt::black);

    this->setPalette(palette);
}

void MediaVideoWidget::paintEvent(QPaintEvent*)
{
    if(image.isNull()) return;

    QPainter painter;
    painter.begin(this);

    qreal ratio1 = image.width() * 1.0 / image.height();
    qreal ratio2 = size().width() * 1.0 / size().height();

    QRect rect = this->rect();
    if(ratio1 > ratio2) {
        rect.setSize({size().width(), (int)(size().width() / ratio1)});
    }else {
        rect.setSize({(int)(size().height() * ratio1), size().height()});
    }

    rect.moveCenter(this->rect().center());

    painter.drawImage(rect, image);
    painter.end();
}
