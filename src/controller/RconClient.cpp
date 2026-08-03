#include "RconClient.h"

#include <QTcpSocket>

RconClient::RconClient(QObject* parent)
    : QObject(parent)
{
    m_socket = new QTcpSocket(this);
    connect(m_socket, &QTcpSocket::connected, this, &RconClient::connected);
    connect(m_socket, &QTcpSocket::disconnected, this, &RconClient::disconnected);
    connect(m_socket, &QTcpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
        emit errorOccurred(m_socket->errorString());
    });
    connect(m_socket, &QTcpSocket::readyRead, this, [this] {
        m_buffer += QString::fromUtf8(m_socket->readAll());
        while (m_buffer.contains(QLatin1Char('\n'))) {
            const int idx = m_buffer.indexOf(QLatin1Char('\n'));
            const QString line = m_buffer.left(idx).trimmed();
            m_buffer.remove(0, idx + 1);
            if (!line.isEmpty())
                emit responseReceived(line);
        }
    });
}

RconClient::~RconClient() = default;

void RconClient::setConnection(const QString& host, quint16 port, const QString& password)
{
    m_host = host;
    m_port = port;
    m_password = password;
}

bool RconClient::isConnected() const
{
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

void RconClient::connectToServer()
{
    if (m_port == 0)
        return;
    if (isConnected())
        return;
    m_buffer.clear();
    m_socket->connectToHost(m_host, m_port);
    if (!m_password.isEmpty() && !m_authArmed) {
        m_authArmed = true;
        // Best-effort auth line for RCON-style mods: password\n
        connect(m_socket, &QTcpSocket::connected, this, [this] {
            m_authArmed = false;
            if (!m_password.isEmpty())
                m_socket->write((m_password + QLatin1Char('\n')).toUtf8());
        }, Qt::SingleShotConnection);
    }
}

void RconClient::disconnectFromServer()
{
    m_socket->disconnectFromHost();
}

void RconClient::send(const QString& command)
{
    if (!isConnected())
        return;
    m_socket->write((command + QLatin1Char('\n')).toUtf8());
}
