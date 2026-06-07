#include "serverworker.h"
#include <QDateTime>
#include <QDebug>
#include <QHostAddress>
#include <QTcpServer>
#include <QTcpSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

ServerWorker::ServerWorker(QObject *parent)
    : QObject(parent)
{ }
// Запуск сервера
void ServerWorker::startServer(quint16 port)
{
    if (m_server) {
        emit logMessage("Сервер уже запущен.");
        return;
    }

    m_server = new QTcpServer(this);

    connect(m_server, &QTcpServer::newConnection, this, &ServerWorker::onNewConnection);

    if (!m_server->listen(QHostAddress::Any, port))
    {
        emit logMessage(QString("Не удалось запустить сервер: %1").arg(m_server->errorString()));
        m_server->deleteLater();
        m_server = nullptr;
        return;
    }
    emit logMessage(QString("Сервер запущен на порту %1").arg(port));
}
// Остановка сервера и отключение клиентов
void ServerWorker::stopServer()
{
    if (m_server) {
        m_server->close();
        m_server->deleteLater();
        m_server = nullptr;
    }

    for (QTcpSocket *socket : m_clients) {
        if (!socket)
            continue;

        socket->disconnectFromHost();
        socket->deleteLater();
    }

    m_clients.clear();
    m_clientActive.clear();

    emit logMessage("Сервер остановлен.");
}
// Запуск для всех клиентов
void ServerWorker::startAllClients()
{
    emit logMessage("Отправка START всем клиентам...");

    for (auto it = m_clients.begin(); it != m_clients.end(); ++it) {
        m_clientActive[it.key()] = true;
        sendCommandToClient(it.key(), "StartCommand");
    }
}
// Остановка всех клиентов
void ServerWorker::stopAllClients()
{
    emit logMessage("Отправка STOP всем клиентам...");

    for (auto it = m_clients.begin(); it != m_clients.end(); ++it) {
        m_clientActive[it.key()] = false;
        sendCommandToClient(it.key(), "StopCommand");
    }
}
// Обновление пороговых значений
void ServerWorker::setCriticalValues(double bandwidth, double latency, int cpu, int memory)
{
    m_criticalBandwidth = bandwidth;
    m_criticalLatency = latency;
    m_criticalCpu = cpu;
    m_criticalMemory = memory;

    emit logMessage(QString("Настройки обновлены: BW=%1 Lat=%2 CPU=%3 MEM=%4")
            .arg(bandwidth)
            .arg(latency)
            .arg(cpu)
            .arg(memory));
}
// Отправка команды клиенту
void ServerWorker::sendCommandToClient( const QUuid &clientId, const QString &command)
{
    if (!m_clients.contains(clientId))
        return;

    QTcpSocket *socket = m_clients.value(clientId);

    if (!socket)
        return;

    QJsonObject cmd;

    cmd["type"] = command;
    cmd["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    socket->write(QJsonDocument(cmd).toJson(QJsonDocument::Compact)+ "\n");
    socket->flush();
}
// Отправка подтверждения получения данных
void ServerWorker::sendAck(QTcpSocket *socket, const QString &message)
{
    if (!socket)
        return;

    QJsonObject ack;

    ack["type"] = "Ack";
    ack["message"] = message;
    socket->write(QJsonDocument(ack).toJson(QJsonDocument::Compact)+ "\n");
    socket->flush();
}
// Обработка нового подключения клиента
void ServerWorker::onNewConnection()
{
    if (!m_server)
        return;

    QTcpSocket *clientSocket = m_server->nextPendingConnection();

    if (!clientSocket)
        return;

    QUuid clientId = QUuid::createUuid();

    m_clients[clientId] = clientSocket;
    m_clientActive[clientId] = false;

    emit logMessage(QString("Подключен клиент %1").arg(clientId.toString()));
    emit clientConnected(clientId.toString(), clientSocket->peerAddress().toString(), "Connected");

    QJsonObject response;

    response["type"] = "ConnectionResponse";
    response["clientId"] = clientId.toString();
    response["message"] = "Welcome to Telecom Server";

    clientSocket->write(QJsonDocument(response).toJson(QJsonDocument::Compact) + "\n");
    clientSocket->flush();

    connect(clientSocket, &QTcpSocket::readyRead, this, &ServerWorker::onReadyRead);
    connect(clientSocket, &QTcpSocket::disconnected, this, &ServerWorker::onClientDisconnected);
    connect(clientSocket, &QTcpSocket::disconnected, clientSocket, &QTcpSocket::deleteLater);
}
// Обработка входящих сообщений клиента
void ServerWorker::onReadyRead()
{
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket*>(sender());

    if (!clientSocket)
        return;

    QUuid clientId;

    for (auto it = m_clients.begin(); it != m_clients.end(); ++it) {
        if (it.value() == clientSocket) {
            clientId = it.key();
            break;
        }
    }

    if (clientId.isNull())
        return;

    while (clientSocket->canReadLine())
    {
        QByteArray line = clientSocket->readLine().trimmed();
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(line, &parseError);

        if (parseError.error != QJsonParseError::NoError) {
            emit logMessage(QString("JSON Error: %1").arg(parseError.errorString()));
            continue;
        }

        if (!doc.isObject())
            continue;

        QJsonObject obj = doc.object();
        QString type = obj["type"].toString();
        QString timeStr = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");

        checkCriticalValues(clientId, obj);
        QString content = QJsonDocument(obj).toJson(QJsonDocument::Compact);

        if (type == "NetworkMetrics") {
            content = QString("BW=%1 Mbps | LAT=%2 ms | LOSS=%3 %")
                    .arg(obj["bandwidth"].toDouble(),0,'f',2)
                    .arg(obj["latency"].toDouble(),0,'f',2)
                    .arg(obj["packet_loss"].toDouble()*100.0,0,'f',2);
        }
        else if (type == "DeviceStatus") {
            content = QString("CPU=%1%% | MEM=%2%% | UPTIME=%3s")
                    .arg(obj["cpu_usage"].toInt())
                    .arg(obj["memory_usage"].toInt())
                    .arg(obj["uptime"].toInt());
        }
        else if (type == "Log") {
            content = QString("[%1] %2")
                    .arg(obj["severity"].toString())
                    .arg(obj["message"].toString());
        }
        emit dataReceived( clientId.toString(), type, content, timeStr);
        sendAck(clientSocket, QString("%1 received").arg(type));
    }
}
// Проверка полученных данных на превышение порогов
void ServerWorker::checkCriticalValues(const QUuid &clientId, const QJsonObject &data)
{
    QString type = data["type"].toString();
    QString alert;

    if (type == "NetworkMetrics") {
        double bw = data["bandwidth"].toDouble();

        double lat = data["latency"].toDouble();

        if (bw > m_criticalBandwidth) {
            alert = QString("HIGH BANDWIDTH: %1").arg(bw);
        }

        if (lat > m_criticalLatency) {
            alert = QString("HIGH LATENCY: %1").arg(lat);
        }
    }
    else if (type == "DeviceStatus") {
        int cpu = data["cpu_usage"].toInt();
        int mem = data["memory_usage"].toInt();

        if (cpu > m_criticalCpu) {
            alert = QString("HIGH CPU: %1").arg(cpu);
        }

        if (mem > m_criticalMemory) {
            alert = QString("HIGH MEMORY: %1").arg(mem);
        }
    }

    if (alert.isEmpty())
        return;

    emit criticalAlert(clientId.toString(), alert);
    emit logMessage(QString("ALERT [%1] %2").arg(clientId.toString()).arg(alert));
    emit dataReceived(clientId.toString(), "Log(ALERT)", alert, QDateTime::currentDateTime().toString("hh:mm:ss.zzz"));
}
// Обработка отключения клиента
void ServerWorker::onClientDisconnected()
{
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket*>(sender());

    if (!clientSocket)
        return;

    QUuid clientId;

    for (auto it = m_clients.begin(); it != m_clients.end(); ++it) {
        if (it.value() == clientSocket) {
            clientId = it.key();
            m_clients.erase(it);
            m_clientActive.remove(clientId);
            break;
        }
    }

    emit logMessage(QString("Клиент отключен: %1").arg(clientId.toString()));
    emit clientDisconnected(clientId.toString());
}