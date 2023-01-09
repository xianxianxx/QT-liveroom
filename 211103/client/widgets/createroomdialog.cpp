#include "createroomdialog.h"
#include "ui_createroomdialog.h"

CreateRoomDialog::CreateRoomDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CreateRoomDialog)
{
    ui->setupUi(this);

    QObject::connect(ui->cbPassword, SIGNAL(clicked(bool)),
                     ui->lePassword, SLOT(setEnabled(bool)));

    QObject::connect(ui->btnCancel, SIGNAL(clicked()),
                     this, SLOT(reject()));
    QObject::connect(ui->btnConfirm, SIGNAL(clicked()),
                     this, SLOT(accept()));
}

CreateRoomDialog::~CreateRoomDialog()
{
    delete ui;
}

QString CreateRoomDialog::getRoomName()
{
    return ui->leRoomname->text();
}

QString CreateRoomDialog::getPassword()
{
    return ui->cbPassword->isChecked() ? ui->lePassword->text() : "";
}

int CreateRoomDialog::getLimit()
{
    return ui->sbLimit->value();
}
