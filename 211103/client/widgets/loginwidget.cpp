#include "loginwidget.h"
#include "ui_loginwidget.h"
#include "clientsocket.h"

#include <QMessageBox>

LoginWidget::LoginWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LoginWidget)
{
    ui->setupUi(this);

    QObject::connect(ui->btnRegister, SIGNAL(clicked()),
                     this, SLOT(onBtnRegisterClicked()));
    QObject::connect(ui->btnLogin, SIGNAL(clicked()),
                     this, SLOT(onBtnLoginClicked()));
}

LoginWidget::~LoginWidget()
{
    delete ui;
}

void LoginWidget::onBtnRegisterClicked()
{
    QString username = ui->leUsername->text();
    QString password = ui->lePassword->text();

    ui->leUsername->clear();
    ui->lePassword->clear();

    if(username.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "注册信息", "注册失败，必须输入账号与密码");
        return;
    }

    SingleTon<ClientSocket>::getReference().userHander()->regist(username, password);
}

void LoginWidget::onBtnLoginClicked()
{
    QString username = ui->leUsername->text();
    QString password = ui->lePassword->text();

    ui->leUsername->clear();
    ui->lePassword->clear();

    if(username.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "登录信息", "登录失败，必须输入账号与密码");
        return;
    }

    SingleTon<ClientSocket>::getReference().userHander()->login(username, password);
}
