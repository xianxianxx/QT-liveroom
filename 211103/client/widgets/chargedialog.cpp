#include "chargedialog.h"
#include "ui_chargedialog.h"

ChargeDialog::ChargeDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ChargeDialog)
{
    ui->setupUi(this);

    QObject::connect(ui->btnConfirm, SIGNAL(clicked()),
                     this, SLOT(accept()));
    QObject::connect(ui->btnCancel, SIGNAL(clicked()),
                     this, SLOT(reject()));
}

ChargeDialog::~ChargeDialog()
{
    delete ui;
}

int ChargeDialog::getMoney() const
{
    return ui->spinBox->value();
}
