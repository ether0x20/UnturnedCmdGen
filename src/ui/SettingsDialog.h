#pragma once

#include <QDialog>

class QComboBox;
class QLineEdit;
class QSpinBox;

class CommandController;
class RconClient;

// Preferences dialog: default output format + optional RCON bridge settings.
// Persisted via QSettings by the controller layer.
class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    SettingsDialog(CommandController* controller, RconClient* rcon, QWidget* parent = nullptr);

private slots:
    void save();

private:
    CommandController* m_controller = nullptr;
    RconClient* m_rcon = nullptr;
    QComboBox* m_format = nullptr;
    QLineEdit* m_host = nullptr;
    QSpinBox* m_port = nullptr;
    QLineEdit* m_password = nullptr;
};
