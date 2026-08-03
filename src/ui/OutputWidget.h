#pragma once

#include <QWidget>

class QButtonGroup;
class QLabel;
class QLineEdit;
class QPushButton;
class QRadioButton;

class CommandController;
class RconClient;

// Bottom panel: output format selector, live command preview, copy to
// clipboard, and optional send-to-server (RCON).
class OutputWidget : public QWidget
{
    Q_OBJECT
public:
    OutputWidget(CommandController* controller, RconClient* rcon, QWidget* parent = nullptr);

private slots:
    void onFormatChanged();
    void copyToClipboard();
    void toggleRcon();
    void sendToServer();

private:
    CommandController* m_controller = nullptr;
    RconClient* m_rcon = nullptr;
    QButtonGroup* m_formatGroup = nullptr;
    QLineEdit* m_preview = nullptr;
    QLabel* m_rconStatus = nullptr;
    QPushButton* m_sendBtn = nullptr;
};
