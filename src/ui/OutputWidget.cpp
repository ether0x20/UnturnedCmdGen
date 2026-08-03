#include "OutputWidget.h"

#include <QApplication>
#include <QButtonGroup>
#include <QClipboard>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QVBoxLayout>

#include "controller/CommandController.h"
#include "controller/RconClient.h"

OutputWidget::OutputWidget(CommandController* controller, RconClient* rcon, QWidget* parent)
    : QWidget(parent)
    , m_controller(controller)
    , m_rcon(rcon)
{
    auto* vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(6, 6, 6, 6);

    // Format selector.
    auto* fmtRow = new QHBoxLayout;
    auto* termBtn = new QRadioButton(tr("Terminal"), this);
    auto* slashBtn = new QRadioButton(tr("Chat (/)"), this);
    auto* atBtn = new QRadioButton(tr("Chat (@)"), this);
    m_formatGroup = new QButtonGroup(this);
    m_formatGroup->addButton(termBtn, static_cast<int>(CommandController::Format::Terminal));
    m_formatGroup->addButton(slashBtn, static_cast<int>(CommandController::Format::ChatSlash));
    m_formatGroup->addButton(atBtn, static_cast<int>(CommandController::Format::ChatAt));
    slashBtn->setChecked(true);
    fmtRow->addWidget(new QLabel(tr("Format:"), this));
    fmtRow->addWidget(termBtn);
    fmtRow->addWidget(slashBtn);
    fmtRow->addWidget(atBtn);
    fmtRow->addStretch(1);
    vbox->addLayout(fmtRow);

    // Preview + actions.
    auto* previewRow = new QHBoxLayout;
    m_preview = new QLineEdit(this);
    m_preview->setReadOnly(true);
    m_preview->setFont(QFont(QStringLiteral("Monospace")));
    m_preview->setStyleSheet(QStringLiteral("background:#2c3e50;color:#ecf0f1;padding:4px;"));
    previewRow->addWidget(m_preview, 1);

    auto* copyBtn = new QPushButton(tr("Copy"), this);
    m_sendBtn = new QPushButton(tr("Send"), this);
    m_sendBtn->setToolTip(tr("Send to the server via RCON/TCP bridge"));
    previewRow->addWidget(copyBtn);
    previewRow->addWidget(m_sendBtn);
    vbox->addLayout(previewRow);

    // RCON status.
    m_rconStatus = new QLabel(tr("RCON: not connected"), this);
    m_rconStatus->setStyleSheet(QStringLiteral("color:#95a5a6;"));
    vbox->addWidget(m_rconStatus);

    connect(m_formatGroup, &QButtonGroup::idClicked, this, &OutputWidget::onFormatChanged);
    connect(copyBtn, &QPushButton::clicked, this, &OutputWidget::copyToClipboard);
    connect(m_sendBtn, &QPushButton::clicked, this, &OutputWidget::sendToServer);
    connect(m_controller, &CommandController::generatedStringChanged, this,
            [this](const QString& text) { m_preview->setText(text); });
    connect(m_rcon, &RconClient::connected, this, [this] {
        m_rconStatus->setText(tr("RCON: connected"));
        m_rconStatus->setStyleSheet(QStringLiteral("color:#27ae60;"));
    });
    connect(m_rcon, &RconClient::disconnected, this, [this] {
        m_rconStatus->setText(tr("RCON: disconnected"));
        m_rconStatus->setStyleSheet(QStringLiteral("color:#95a5a6;"));
    });
    connect(m_rcon, &RconClient::errorOccurred, this, [this](const QString& msg) {
        m_rconStatus->setText(tr("RCON error: %1").arg(msg));
        m_rconStatus->setStyleSheet(QStringLiteral("color:#c0392b;"));
    });
}

void OutputWidget::onFormatChanged()
{
    const int id = m_formatGroup->checkedId();
    m_controller->setFormat(static_cast<CommandController::Format>(id));
}

void OutputWidget::copyToClipboard()
{
    QApplication::clipboard()->setText(m_controller->generatedString());
}

void OutputWidget::toggleRcon()
{
    if (m_rcon->isConnected())
        m_rcon->disconnectFromServer();
    else
        m_rcon->connectToServer();
}

void OutputWidget::sendToServer()
{
    if (!m_rcon->isConnected())
        m_rcon->connectToServer();
    if (!m_controller->generatedString().isEmpty())
        m_rcon->send(m_controller->generatedString());
}
