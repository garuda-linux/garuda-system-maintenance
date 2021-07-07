#include "settingsdialog.h"
#include "ui_settingsdialog.h"

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
    this->setAttribute(Qt::WA_DeleteOnClose);
    ui->updateKeyrings->setChecked(settings.value("app/updatekeyrings", true).toBool());
    ui->updateHotfixes->setChecked(settings.value("app/updatehotfixes", true).toBool());
    ui->notifyForum->setChecked(settings.value("app/notifyforum", true).toBool());
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::on_buttonBox_rejected()
{
    this->close();
}


void SettingsDialog::on_buttonBox_accepted()
{
    settings.setValue("app/updatekeyrings", ui->updateKeyrings->isChecked());
    settings.setValue("app/updatehotfixes", ui->updateHotfixes->isChecked());
    settings.setValue("app/notifyforum", ui->notifyForum->isChecked());
    this->close();
}


void SettingsDialog::on_updateKeyrings_toggled(bool checked)
{
    ui->updateHotfixes->setEnabled(checked);
}

