#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QSettings>

namespace Ui {
class SettingsDialog;
}

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QObject* parent = nullptr);
    ~SettingsDialog();

private slots:
    void on_buttonBox_rejected();
    void on_buttonBox_accepted();

    void on_updateKeyrings_toggled(bool checked);

private:
    Ui::SettingsDialog* ui;
    QSettings settings;
};

#endif // SETTINGSDIALOG_H
