#include "serverwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTableWidget>
#include <QTextEdit>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QHeaderView>
#include <QMetaObject>
/////// Исправление 4 ПКМ-ребут
//  #include <QMenu>
//  #include <QAction>

// Создание интерфейса и запуск рабочего потока сервера
ServerWindow::ServerWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUI();

    m_thread = new QThread(this);
    m_worker = new ServerWorker();
    m_worker->moveToThread(m_thread);

    connect(m_btnStart, &QPushButton::clicked, this,
        [this]() {
            if (!m_thread->isRunning())
                m_thread->start();

            QMetaObject::invokeMethod( m_worker, "startServer", Qt::QueuedConnection, Q_ARG(quint16, 12345));
        });

    connect(m_btnStop, &QPushButton::clicked, this,
        [this]() {
            QMetaObject::invokeMethod( m_worker, "stopServer", Qt::QueuedConnection);
        });

    connect(m_worker, &ServerWorker::logMessage, this,
        [this](const QString &msg) {
            m_logEdit->append(msg);
        });

    connect(
        m_worker,
        &ServerWorker::criticalAlert,
        this,
        [this](const QString &clientId, const QString &alert) {
            m_logEdit->append(QString("🚨 ALERT [%1]: %2").arg(clientId, alert)); });

    connect(m_worker, &ServerWorker::clientConnected, this, &ServerWindow::updateClientsTable);

    connect(m_worker, &ServerWorker::dataReceived, this, &ServerWindow::updateDataTable);

    connect(m_worker, &ServerWorker::clientDisconnected, this,
        [this](const QString &id) { updateClientsTable( id, "", "Disconnected"); });

    resize(1200, 700);

    setWindowTitle("Telecom Server");
}

// Корректное завершение потока сервера
ServerWindow::~ServerWindow()
{
    if (m_thread) {
        if (m_thread->isRunning()) {
            QMetaObject::invokeMethod( m_worker, "stopServer", Qt::BlockingQueuedConnection);
            m_thread->quit();
            m_thread->wait();
        }
    }
}
// Настройка графического интерфейса
void ServerWindow::setupUI()
{
    QWidget *central = new QWidget(this);
    auto *mainLayout = new QVBoxLayout(central);

// Кнопки управление сервером

    auto *serverButtons = new QHBoxLayout();

    m_btnStart = new QPushButton("Start Server");
    m_btnStop = new QPushButton("Stop Server");

    serverButtons->addWidget(m_btnStart);
    serverButtons->addWidget(m_btnStop);

    mainLayout->addLayout(serverButtons);

// Кнопки управления клиентами
    auto *clientButtons = new QHBoxLayout();

    auto *btnStartClients = new QPushButton("Start All Clients");

    auto *btnStopClients = new QPushButton("Stop All Clients");

    clientButtons->addWidget(btnStartClients);

    clientButtons->addWidget(btnStopClients);

    mainLayout->addLayout(clientButtons);

    connect(btnStartClients, &QPushButton::clicked, this,
        [this]() {
            QMetaObject::invokeMethod(
                m_worker,
                "startAllClients",
                Qt::QueuedConnection);
        });

    connect(btnStopClients, &QPushButton::clicked, this,
        [this]() {
            QMetaObject::invokeMethod(
                m_worker,
                "stopAllClients",
                Qt::QueuedConnection);
        });

// Настройка критических значений
    auto *settingsGroup = new QGroupBox("Critical Values");
    auto *settingsLayout = new QHBoxLayout();
    auto *editBandwidth = new QLineEdit("900");
    auto *editLatency = new QLineEdit("40");
    auto *editCpu = new QLineEdit("80");
    auto *editMemory = new QLineEdit("85");
    auto *applyButton = new QPushButton("Apply");

    settingsLayout->addWidget(new QLabel("Bandwidth"));
    settingsLayout->addWidget(editBandwidth);

    settingsLayout->addWidget(new QLabel("Latency"));
    settingsLayout->addWidget(editLatency);

    settingsLayout->addWidget(new QLabel("CPU"));
    settingsLayout->addWidget(editCpu);

    settingsLayout->addWidget(new QLabel("Memory"));
    settingsLayout->addWidget(editMemory);

    settingsLayout->addWidget(applyButton);
    settingsGroup->setLayout(settingsLayout);

    mainLayout->addWidget(settingsGroup);

    connect(applyButton, &QPushButton::clicked, this,
        [this, editBandwidth, editLatency, editCpu, editMemory]()
        {
            QMetaObject::invokeMethod(
                m_worker,
                "setCriticalValues",
                Qt::QueuedConnection,
                Q_ARG(double, editBandwidth->text().toDouble()),
                Q_ARG(double, editLatency->text().toDouble()),
                Q_ARG(int, editCpu->text().toInt()),
                Q_ARG(int, editMemory->text().toInt()));
        });

// Таблицы мониторинга
    auto *tablesLayout = new QHBoxLayout();
    m_clientsTable = new QTableWidget(0, 3);
    m_clientsTable->setHorizontalHeaderLabels({"Client ID", "IP", "Status"});
    m_clientsTable->horizontalHeader()->setStretchLastSection(true);

/////// Исправление 4 ПКМ-ребут
    /*
    m_clientsTable->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(m_clientsTable, &QWidget::customContextMenuRequested, this, [this](const QPoint& pos) {
        // Узнаём строку клика
        int row = m_clientsTable->rowAt(pos.y());
        if (row < 0) return;

        // Достаём ID
        QTableWidgetItem* itemId = m_clientsTable->item(row, 0);
        if (!itemId) return;
        QString id = itemId->text();

        // Создаём меню
        QMenu menu(this);
        QAction* rebootAction = menu.addAction("Reboot Device");

        // Показываем меню и ждём клик
        QAction* chosen = menu.exec(QCursor::pos());

        //Реагируем на выбор
        if (chosen == rebootAction)
            QMetaObject::invokeMethod(m_worker, "sendRebootToClient", Qt::QueuedConnection, Q_ARG(QString, id));
        });
        */
/////// Исправление 4 ПКМ-ребут


    m_dataTable = new QTableWidget(0, 4);
    m_dataTable->setHorizontalHeaderLabels( {"Client ID", "Type", "Content", "Time"});
    m_dataTable->horizontalHeader()->setStretchLastSection(true);

    tablesLayout->addWidget(m_clientsTable);
    tablesLayout->addWidget(m_dataTable);

    mainLayout->addLayout(tablesLayout);

// Логи
    m_logEdit = new QTextEdit();
    m_logEdit->setReadOnly(true);
    mainLayout->addWidget(m_logEdit);
    setCentralWidget(central);
}
// Обновление информации о клиентах
void ServerWindow::updateClientsTable(const QString &id, const QString &ip, const QString &status)
{
    int row = -1;

    for (int i = 0; i < m_clientsTable->rowCount(); ++i)
    {
        if (m_clientsTable->item(i, 0) && m_clientsTable->item(i, 0)->text() == id) {
            row = i;
            break;
        }
    }

    if (row == -1) {
        row = m_clientsTable->rowCount();
        m_clientsTable->insertRow(row);
    }

    m_clientsTable->setItem(row, 0, new QTableWidgetItem(id));
    m_clientsTable->setItem(row, 1, new QTableWidgetItem(ip));
    m_clientsTable->setItem(row, 2, new QTableWidgetItem(status));
}
// Добавление записи о полученных данных
void ServerWindow::updateDataTable( const QString &id, const QString &type, const QString &content, const QString &time)
{
    int row = m_dataTable->rowCount();
    m_dataTable->insertRow(row);

    m_dataTable->setItem(row, 0, new QTableWidgetItem(id));
    m_dataTable->setItem(row, 1, new QTableWidgetItem(type));
    m_dataTable->setItem(row, 2, new QTableWidgetItem(content));
    m_dataTable->setItem(row, 3, new QTableWidgetItem(time));

    m_dataTable->scrollToBottom();
}