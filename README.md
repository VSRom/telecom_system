### Telecom System

Qt 6.5.2. 

A client-server application for simulating telecommunication devices. 

### Server

* GUI application based on Qt Widgets
* Listens on port 12345
* Supports multiple clients
* Displays connected clients and received data
* Allows configuring critical threshold values for metrics
* Generates warnings when thresholds are exceeded

### Client

* Console-based device emulator
* Connects to localhost:12345
* Automatically reconnects if the connection is lost
* Sends the following data types: 

  * Network Metrics
  * Device Status
  * Log

### Technologies Used

* Qt 6.5.2
* QTcpServer
* QTcpSocket
* QJsonDocument
* QJsonObject
* QThread
* QTableWidget

### Build

### Qt Creator

1. Open the project in Qt Creator
2. Perform Configure Project
3. Build the project

### CMake

bash

mkdir build
cd build
cmake ..
cmake --build . --config Release

Используйте код с осторожностью.

### Running the Application

1. Run Server
2. Click **Start Server**
3. Run one or multiple instances of Client
4. Click **Start All Clients**
