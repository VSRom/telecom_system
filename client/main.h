#pragma once

#include <QCoreApplication>
#include <QTcpSocket>
#include <QTimer>
#include <QString>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include <QRandomGenerator>
#include <QDebug>
#include <QDateTime>
#include <QAbstractSocket>
////////Исправление 1 перевод на ОЧЕРЕДЬ! Для не потери данных!
// #include <QQueue>
/////////// Исправление 3 QDataStream
// #include <QDataStream>

class DeviceEmulator : public QObject
{
    Q_OBJECT

public:
    explicit DeviceEmulator(QObject *parent = nullptr)
        : QObject(parent), m_socket(new QTcpSocket(this)), m_isConnected(false), m_waitingForStart(true)
{
        m_startTime = QDateTime::currentSecsSinceEpoch();
        connect(m_socket, &QTcpSocket::connected, this, &DeviceEmulator::onConnected);
        connect(m_socket, &QTcpSocket::disconnected, this, &DeviceEmulator::onDisconnected);
        connect(m_socket, &QTcpSocket::errorOccurred, this, &DeviceEmulator::onError);
        connect(m_socket, &QTcpSocket::readyRead, this, &DeviceEmulator::onReadyRead);
        connect(&m_timer, &QTimer::timeout, this, &DeviceEmulator::tryConnect);
        connect(&m_sendTimer, &QTimer::timeout, this, &DeviceEmulator::sendRandomData);
        tryConnect();
    }

private slots:
// Подключение к серверу
    void tryConnect()
    {
        if (m_socket->state() != QAbstractSocket::UnconnectedState)
            return;

        qDebug() << "[Client] Attempting to connect to localhost:12345...";

        m_socket->connectToHost("localhost", 12345);
    }
// Обработка успешного подключения
    void onConnected()
    {
        qDebug() << "[Client] Successfully connected to server.";
        m_isConnected = true;
        m_waitingForStart = true;
        m_timer.stop();
        
/////////Исправление 1 перевод на ОЧЕРЕДЬ! Для не потери данных!
        /*
        while (!m_offQueue.isEmpty())
            m_socket->write(m_offQueue.dequeue());

        m_socket->flush();
        qDebug() << "[Client] Queueu flushed.";
        */
/////////Исправление 1 перевод на ОЧЕРЕДЬ! Для не потери данных!

    }
// Обработка потери соединения
    void onDisconnected()
    {
        qDebug() << "[Client] Connection lost. Reconnecting in 5 seconds...";

        m_isConnected = false;
        m_waitingForStart = true;

        m_sendTimer.stop();

        m_timer.start(5000);
    }

    void onError(QAbstractSocket::SocketError) {
        qDebug() << "[Client] Socket error:" << m_socket->errorString();
    }
// Обработка сообщений от сервера
    void onReadyRead()
    {
        while (m_socket->canReadLine())
        {
            QByteArray line = m_socket->readLine().trimmed();

            if (line.isEmpty())
                continue;

            QJsonParseError parseError;

            QJsonDocument doc = QJsonDocument::fromJson(line, &parseError);

            if (parseError.error != QJsonParseError::NoError) {
                qDebug() << "[Client] Invalid JSON:" << parseError.errorString();
                continue;
            }

            if (!doc.isObject())
                continue;

            QJsonObject obj = doc.object();

            QString type = obj["type"].toString();

            if (type == "ConnectionResponse")
            {
                m_clientId = obj["clientId"].toString();
                qDebug() << "[Client] Connection confirmed.";
                qDebug() << "[Client] Client ID:" << m_clientId;
                qDebug() << "[Client] Message:" << obj["message"].toString();
            }
            else if (type == "StartCommand")
            {
                qDebug() << "[Client] START command received.";
                m_waitingForStart = false;
                startSendingData();
            }
            else if (type == "StopCommand")
            {
                qDebug() << "[Client] STOP command received.";
                m_waitingForStart = true;
                m_sendTimer.stop();
            }
            else if (type == "Ack")
            {
                qDebug() << "[Client] Server ACK:" << obj["message"].toString();
            }
/////// Исправление 4 ПКМ-ребут
            /*
            else if (type == "RebootCommand") {
                qDebug() << "[Client] REBOOT command received. Emulating restart...";
                m_waitingForStart = true;
                m_sendTimer.stop();

                QTimer::singleShot(2000, this, [this]() {
                    qDebug() << "[Client] Device restarted.";
                    });
            }
            */
/////// Исправление 4 ПКМ-ребут
        }
    }
// Запуск периодической отправки данных
    void startSendingData()
    {
        int delay = QRandomGenerator::global()->bounded(10, 100);
        m_sendTimer.start(delay);
    }
// Генерация и отправка случайных данных
    void sendRandomData()
    {
        if (m_waitingForStart || !m_isConnected)
            return;

        QJsonObject data;

        int type = QRandomGenerator::global()->bounded(3);

        if (type == 0)
        {
            data["type"] = "NetworkMetrics";
            data["bandwidth"] = QRandomGenerator::global()->generateDouble() * 1000.0;
            data["latency"] = QRandomGenerator::global()->generateDouble() * 50.0;
            data["packet_loss"] = QRandomGenerator::global()->generateDouble() * 0.1;
        }
        else if (type == 1)
        {
            data["type"] = "DeviceStatus";
            data["uptime"] = QDateTime::currentSecsSinceEpoch() - m_startTime;
            data["cpu_usage"] = QRandomGenerator::global()->bounded(100);
            data["memory_usage"] = QRandomGenerator::global()->bounded(100);
        }
        else
        {
            data["type"] = "Log";
            data["severity"] = QRandomGenerator::global()->bounded(2) ? "INFO" : "WARNING";

            int lengthType = QRandomGenerator::global()->bounded(3);

            if (lengthType == 0)
            {
                data["message"] = QString("Short log %1").arg(QRandomGenerator::global()->bounded(1000));
            }
            else if (lengthType == 1)
            {
                data["message"] = QString("Medium log message with some details about event %1 in module %2")
                        .arg(QRandomGenerator::global()->bounded(10000))
                        .arg(QRandomGenerator::global()->bounded(100));
            }
            else
            {
                QString longMsg =
                    QString("Long log message: Detailed information about network event %1. ")
                        .arg(QRandomGenerator::global()->bounded(100000));

                longMsg += "Additional context: The system detected unusual activity on interface eth0. ";
                longMsg += "Diagnostic data shows increased latency and packet loss rates. ";
                longMsg += "Recommendation: Check network configuration and firewall rules.";

                data["message"] = longMsg;
            }
        }
////////////////////////////////////============================/////////////////////////////////////////
        QJsonDocument doc(data);
        // БЫЛО
        //  QByteArray json = doc.toJson(QJsonDocument::Compact) + "\n";
        //  m_socket->write(json);


////////////////////////////////////============================/////////////////////////////////////////
////////// Исправление 1 перевод на ОЧЕРЕДЬ! Для не потери данных!
        /*
        QByteArray json = doc.toJson(QJsonDocument::Compact) + "\n";

        if (m_isConnected && m_socket->state() == QAbstractSocket::ConnectedState) {
            m_socket->write(json);
            m_socket->flush();
        }
        else {
            m_offQueue.enqueue(json);
            qDebug() << "[Client] Server down. Packet queued. Size: " << m_offQueue.size();
        }
        */
/////////// Исправление 1 перевод на ОЧЕРЕДЬ! Для не потери данных!
////////////////////////////////////============================/////////////////////////////////////////
/////////// Исправление 3 QDataStream
        /*
        QByteArray json = doc.toJson(QJsonDocument::Compact);
        // Создаём пакет 4 байта + JSON
        QByteArray packet;
        QDataStream out(&packet, QIODevice::WriteOnly);
        // Фиксация версии ?
        out.setVersion(QDataStream::Qt_6_4);

        // Сначала размер потом данные
        out << static_cast<quint32>(json.size());
        packet.append(json);
        m_socket->write(packet);
        m_socket->flush();
        */
/////////// Исправление 3 QDataStream
////////////////////////////////////============================/////////////////////////////////////////
        qDebug() << "[Client] Sent:" << data["type"].toString();
    }

private:
    QTcpSocket *m_socket;
    QTimer m_timer;
    QTimer m_sendTimer;
    QString m_clientId;
    qint64 m_startTime;
    bool m_isConnected;
    bool m_waitingForStart;
///////Исправление 1 перевод на ОЧЕРЕДЬ! Для не потери данных!
    // QQueue<QByteArray> m_offQueue;  // Храним наборы байтов в очереди
};
