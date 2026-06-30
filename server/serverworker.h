#pragma once

// Qt Core
#include <QObject>
#include <QString>
#include <QUuid>
#include <QMap>
/////////// Исправление 3 QDataStream
//  #include <QDataStream>

// Qt Network
#include <QTcpServer>
#include <QTcpSocket>
/////// Исправление 2 приём клиентов из 1 подсети
//  #include <QHostAddress>

// Qt JSON
#include <QJsonObject>

class ServerWorker : public QObject
{
    Q_OBJECT

public:
    explicit ServerWorker(QObject *parent = nullptr);

public slots:
    void startServer(quint16 port);
    void stopServer();

    void startAllClients();
    void stopAllClients();

    void setCriticalValues(double bandwidth, double latency, int cpu, int memory);

signals:
    void clientConnected(const QString &id, const QString &ip, const QString &status);

    void clientDisconnected(const QString &id);

    void dataReceived(const QString &clientId, const QString &type, const QString &content, const QString &time);

    void logMessage(const QString &msg);

    void criticalAlert(const QString &clientId, const QString &message);

private slots:
    void onNewConnection();
    void onReadyRead();
    void onClientDisconnected();

    /////// Исправление 4 ПКМ-ребут
    //  void sendRebootToClient(const QString& clientIdStr);

private:
    void sendCommandToClient(const QUuid &clientId, const QString &command);

    void sendAck(QTcpSocket *socket, const QString &message);

    void checkCriticalValues(const QUuid &clientId, const QJsonObject &data);

private:
    QTcpServer *m_server = nullptr;
    QMap<QUuid, QTcpSocket*> m_clients;
    QMap<QUuid, bool> m_clientActive;
    double m_criticalBandwidth = 900.0;
    double m_criticalLatency = 40.0;
    int m_criticalCpu = 80;
    int m_criticalMemory = 85;
/////////// Исправление 3 QDataStream
   //   QMap<QTcpSocket*, quint32> m_pendingSize;    // Храним размер для каждого сокета
   //   void sendBinaryPacket(QTcpSocket* socket, const QJsonObject& obj);  // Все отправки на клиента

/////// Исправление 2 приём клиентов из 1 подсети    // Мы тут будем проверять адреса от 1.1 до 1.254
    //  QHostAddress m_allowedSubnet{ "192.168.1.0" };  // Адрес подсети
    //  int m_netmask = 24; // mask 255.255.255.0
};