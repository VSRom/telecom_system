# telecom system

Qt 6.5.2.

Клиент-серверное приложение для эмуляции телекоммуникационных устройств.

## Сервер

- GUI-приложение на Qt Widgets
- Прослушивает порт 12345
- Поддерживает несколько клиентов
- Отображает клиентов и получаемые данные
- Позволяет настраивать критические значения метрик
- Формирует предупреждения при превышении порогов

## Клиент

- Консольный эмулятор устройства
- Подключается к localhost:12345
- Автоматически переподключается при потере соединения
- Отправляет данные типов:
  - Network Metrics
  - Device Status
  - Log

## Используемые технологии

- Qt 6.5.2
- QTcpServer
- QTcpSocket
- QJsonDocument
- QJsonObject
- QThread
- QTableWidget

## Сборка

### Qt Creator

1. Открыть проект в Qt Creator
2. Выполнить Configure Project
3. Собрать проект

### CMake

```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```
## Запуск

1. Запустить Server
2. Нажать **Start Server**
3. Запустить один или несколько экземпляров Client
4. Нажать **Start All Clients**