#ifndef BARRAGE_H
#define BARRAGE_H

#include <QObject>
#include <QList>
#include <QLabel>
#include <QTimer>

class BarrageManager : public QObject
{
    Q_OBJECT
public:
    BarrageManager(QWidget* widget, QObject* parent = nullptr);
    ~BarrageManager();

    void append(const QString& barrage);

private slots:
    void onTimeout();

private:
    QWidget* widget;
    QTimer* timer;

    QList<QLabel*> labels;
    int y;
};

#endif // BARRAGE_H
