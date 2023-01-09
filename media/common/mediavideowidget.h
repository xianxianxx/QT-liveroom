#ifndef MEDIAVIDEOWIDGET_H
#define MEDIAVIDEOWIDGET_H

#include <QWidget>

class MediaVideoWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MediaVideoWidget(QWidget *parent = nullptr);

    inline void setImage(const QImage& image){ this->image = image; repaint(); }
    inline void setImage(QImage&& image){ this->image = std::move(image); repaint(); }
signals:

protected:
    virtual void paintEvent(QPaintEvent*) override;

private:
    QImage image;
};

#endif // MEDIAVIDEOWIDGET_H
