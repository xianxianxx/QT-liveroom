#ifndef VIDEOWIDGET_H
#define VIDEOWIDGET_H

#include <QObject>
#include <QWidget>
#include <QImage>

class VideoWidget : public QWidget
{
    Q_OBJECT
public:
    explicit VideoWidget(QWidget *parent = nullptr);

    inline void setImage(const QImage& image){ this->image = image; repaint(); }
    inline void setImage(QImage&& image) { this->image = std::move(image); repaint(); }

protected:
    virtual void paintEvent(QPaintEvent *) override;

signals:

private:
    QImage image;
};

#endif // VIDEOWIDGET_H
