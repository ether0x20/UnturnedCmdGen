#include "SettingsDialog.h"

#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include "controller/CommandController.h"
#include "controller/RconClient.h"

SettingsDialog::SettingsDialog(CommandController* controller, RconClient* rcon, QWidget* parent)
    : QDialog(parent)
    , m_controller(controller)
    , m_rcon(rcon)
{
    setWindowTitle(tr("Settings"));
    auto* vbox = new QVBoxLayout(this);
    auto* form = new QFormLayout;

    m_format = new QComboBox(this);
    m_format->addItem(tr("Terminal"), static_cast<int>(CommandController::Format::Terminal));
    m_format->addItem(tr("Chat (/)"), static_cast<int>(CommandController::Format::ChatSlash));
    m_format->addItem(tr("Chat (@)"), static_cast<int>(CommandController::Format::ChatAt));
    m_format->setCurrentIndex(m_format->findData(static_cast<int>(m_controller->format())));
    form->addRow(tr("Default format"), m_format);

    m_host = new QLineEdit(QStringLiteral("127.0.0.1"), this);
    m_port = new QSpinBox(this);
    m_port->setRange(1, 65535);
    m_port->setValue(27015);
    m_password = new QLineEdit(this);
    m_password->setEchoMode(QLineEdit::Password);

    form->addRow(tr("RCON host"), m_host);
    form->addRow(tr("RCON port"), m_port);
    form->addRow(tr("RCON password"), m_password);
    vbox->addLayout(form);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Save)->setText(tr("Save"));
    buttons->button(QDialogButtonBox::Cancel)->setText(tr("Cancel"));
    vbox->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &SettingsDialog::save);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void SettingsDialog::save()
{
    m_controller->setFormat(static_cast<CommandController::Format>(m_format->currentData().toInt()));
    m_rcon->setConnection(m_host->text().trimmed(), m_port->value(), m_password->text());
    accept();
}
