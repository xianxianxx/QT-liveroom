#include "passworddialog.h"
#include "ui_passworddialog.h"

PasswordDialog::PasswordDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PasswordDialog)
{
    ui->setupUi(this);

    QObject::connect(ui->btnConfirm, SIGNAL(clicked()),
                     this, SLOT(accept()));
    QObject::connect(ui->btnCancel, SIGNAL(clicked()),
                     this, SLOT(reject()));
    QObject::connect(ui->lePassword, SIGNAL(textChanged(const QString&)),
                     this, SLOT(onTextChanged(const QString&)));
}

PasswordDialog::~PasswordDialog()
{
    delete ui;
}

QString PasswordDialog::getPassword() const
{
    return ui->lePassword->text();
}

void PasswordDialog::onTextChanged(const QString& text)
{
    ui->btnConfirm->setDisabled(text.isEmpty());
}
