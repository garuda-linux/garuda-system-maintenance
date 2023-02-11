#include "settingsdialog.h"
#include "ui_settingsdialog.h"

SettingsDialog::SettingsDialog(QObject* parent)
    : ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
    this->setAttribute(Qt::WA_DeleteOnClose);
    ui->updateKeyrings->setChecked(settings.value("application/updatekeyrings", true).toBool());
    ui->updateHotfixes->setChecked(settings.value("application/updatehotfixes", true).toBool());
    ui->notifyForum->setChecked(settings.value("application/notifyforum", true).toBool());
    ui->notifyPartialUpgrade->setChecked(settings.value("application/partialupgrade", true).toBool());
    ui->notifyOutOfDate->setChecked(settings.value("application/outofdate", true).toBool());
    ui->notifyOldSnapshot->setChecked(settings.value("application/oldsnapshot", true).toBool());
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
    settings.setValue("application/updatekeyrings", ui->updateKeyrings->isChecked());
    settings.setValue("application/updatehotfixes", ui->updateHotfixes->isChecked());
    settings.setValue("application/notifyforum", ui->notifyForum->isChecked());
    settings.setValue("application/partialupgrade", ui->notifyPartialUpgrade->isChecked());
    settings.setValue("application/outofdate", ui->notifyOutOfDate->isChecked());
    settings.setValue("application/oldsnapshot", ui->notifyOldSnapshot->isChecked());
    this->close();
}

void SettingsDialog::on_updateKeyrings_toggled(bool checked)
{
    ui->updateHotfixes->setEnabled(checked);
}
