#pragma once

#include <QMainWindow>
#include <QThread>

#include <QTableWidget>
#include <QTextEdit>
#include <QPushButton>

#include "serverworker.h"

class ServerWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ServerWindow(QWidget *parent = nullptr);
    ~ServerWindow();

private:
    void setupUI();

private slots:
    void updateClientsTable(const QString &id, const QString &ip, const QString &status);

    void updateDataTable(const QString &id, const QString &type, const QString &content, const QString &time);

private:
    ServerWorker *m_worker = nullptr;
    QThread *m_thread = nullptr;
    QTableWidget *m_clientsTable = nullptr;
    QTableWidget *m_dataTable = nullptr;
    QTextEdit *m_logEdit = nullptr;
    QPushButton *m_btnStart = nullptr;
    QPushButton *m_btnStop = nullptr;
};