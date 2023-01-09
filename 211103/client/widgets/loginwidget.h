#ifndef LOGINWIDGET_H
#define LOGINWIDGET_H

#include <QWidget>

namespace Ui {
class LoginWidget;
}

class LoginWidget : public QWidget
{
friend class WidgetManager;

    Q_OBJECT

private:
    explicit LoginWidget(QWidget *parent = nullptr);
    LoginWidget(const LoginWidget&)=delete;
    LoginWidget(LoginWidget&&)=delete;
    LoginWidget& operator=(const LoginWidget&)=delete;
    LoginWidget& operator=(LoginWidget&&)=delete;

public:
    ~LoginWidget();

private slots:
    void onBtnRegisterClicked();
    void onBtnLoginClicked();

private:
    Ui::LoginWidget *ui;
};

#endif // LOGINWIDGET_H
