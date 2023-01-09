#include "barrage.h"

#include <QDebug>

BarrageManager::BarrageManager(QWidget* widget, QObject* parent)
    : QObject(parent), widget(widget), y(1)
{
    timer = new QTimer(this);

    QObject::connect(timer, SIGNAL(timeout()),
                     this, SLOT(onTimeout()));
}

BarrageManager::~BarrageManager()
{
    timer->stop();
    for(auto label : labels) label->deleteLater();
    labels.clear();
}

void BarrageManager::append(const QString& barrage)
{
    // 1. 创建标签
    QLabel* label = new QLabel(barrage, widget);
    label->move(widget->width(), 50 * y);
    if(++y > 5) y =1;

    label->show();

    labels.append(label);

    if(labels.size() == 1) {
        // 有新的标签
        timer->start(10);
    }
}

void BarrageManager::onTimeout()
{
    for(auto it = labels.begin(); it != labels.end();) {
        // 往左边漂
        QPoint point = (*it)->pos() + QPoint(-1, 0);
        if(point.x() < -(*it)->width()) {
            // 漂出去了
            (*it)->deleteLater();
            it = labels.erase(it);
            continue;
        }

        (*it)->move(point);
        ++it;
    }

    if(labels.isEmpty()) timer->stop();
}
