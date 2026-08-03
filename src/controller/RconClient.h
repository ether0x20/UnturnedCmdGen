#pragma once

#include <QObject>
#include <QString>

class QTcpSocket;

// Minimal TCP command sender. Unturned has no built-in RCON, so this is a
// generic line-based client that works with RCON-style mods or a telnet
// bridge to the server console. Optional and independent of the core app.
class RconClient : public QObject
{
    Q_OBJECT
public:
    explicit RconClient(QObject* parent = nullptr);
    ~RconClient() override;

    void setConnection(const QString& host, quint16 port, const QString& password);
    bool isConnected() const;

public slots:
    void connectToServer();
    void disconnectFromServer();
    void send(const QString& command);

signals:
    void connected();
    void disconnected();
    void errorOccurred(const QString& message);
    void responseReceived(const QString& text);

private:
    QTcpSocket* m_socket = nullptr;
    QString m_host;
    quint16 m_port = 0;
    QString m_password;
    QString m_buffer;
    bool m_authArmed = false;
};
