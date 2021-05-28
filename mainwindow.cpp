#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTime>
#include <QFile>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    tcpClient = new QTcpSocket(this);
    tcpClient->abort();
    connect(tcpClient, SIGNAL(readyRead()), SLOT(ReadData()));
    connect(tcpClient, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(ReadError(QAbstractSocket::SocketError)));
    connect(&m_timer, &QTimer::timeout, this, &MainWindow::handleTimeout);
    m_timer.setInterval(200);
    // 配置文件，保存IP地址和端口
    QFile file("conf.ini");
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QByteArray t = file.readAll();
    QString str(t);
    file.close();
    if (!t.isEmpty())
    {
        QStringList lst = str.split(":");
        if (lst.size() == 2)
        {
            ui->EditIP->setText(lst[0]);
            ui->EditPort->setText(lst[1]);
        }
    }

//    tcpClient->connectToHost(ui->EditIP->text(), ui->EditPort->text().toUShort());
//    if (tcpClient->waitForConnected(1000))  // 连接成功则进入if{}
//    {
//        ui->BtnConn->setText("断开连接");
//        ui->BtnSend->setEnabled(true);
//    }
//    else
//    {
//        ui->BtnConn->setText("连接服务器");
//        ui->BtnSend->setEnabled(false);
//    }

    ui->BtnConn->setText("连接服务器");
    ui->BtnSend->setEnabled(false); // 发送按钮未使能
    ui->radioClient->setChecked(true);

    tcpServer = new QTcpServer(this);
    // 当有新的客户端接入时，触发NewConnectionSlot函数
    connect(tcpServer, SIGNAL(newConnection()), this, SLOT(NewConnectionSlot()));
    ui->cbLstClients->setVisible(false);

//    showMaximized();

    myChart.createNewSerie("temp", Qt::red, 10, 50);
    myChart.createNewSerie("ax", Qt::black, -20000, 20000);
    myChart.createNewSerie("ay", Qt::blue, -20000, 20000);
    myChart.createNewSerie("az", Qt::cyan, -20000, 20000);
    myChart.createNewSerie("gx", Qt::gray, -2000, 2000);
    myChart.createNewSerie("gy", Qt::green,  -2000, 2000);
    myChart.createNewSerie("gz", Qt::magenta,  -2000, 2000);
    myChart.createNewSerie("pitch", Qt::yellow,  -180, 180);
    myChart.createNewSerie("roll", Qt::darkBlue, -180, 180);
    myChart.createNewSerie("yaw", Qt::darkGreen, -180, 180);
    ui->chartWidget->setChart(&myChart);
    ui->chartWidget->setRenderHint(QPainter::Antialiasing);

}

MainWindow::~MainWindow()
{
    if (tcpServer->isListening()) {
        for(int i = lstClient.length() - 1; i >= 0; --i) //断开所有连接
        {
            lstClient[i]->disconnectFromHost();
            lstClient.removeAt(i);  //从保存的客户端列表中取去除
        }
        tcpServer->close();     //不再监听端口
    }

    if (tcpClient->state() == QAbstractSocket::ConnectedState)
    {
        tcpClient->abort();
        tcpClient->disconnectFromHost();
        tcpClient->close();
    }
    delete ui;
}

void MainWindow::GetLocalIPAddress()
{
    QList<QHostAddress> lst = QNetworkInterface().allAddresses();
    for (int i = 0; i < lst.size(); ++i)
    {
        QHostAddress tha = lst[i];
        QString tstr = tha.toString();
        if (tha.isNull() || tha.isLoopback() || tha.protocol() != QAbstractSocket::IPv4Protocol) // IP地址可能会有问题，缺少一个条件
            continue;
        else
        {
            ui->EditIP->setText(lst[i].toString());   // 显示本地IP地址
            break;
        }
    }
}

// 用于16进制发送时转为二进制数据
QByteArray MainWindow::HexStringToByteArray(QString HexString)
{
    bool ok;
    QByteArray ret;
    HexString = HexString.trimmed();
    HexString = HexString.simplified();
    QStringList sl = HexString.split(" ");

    foreach (QString s, sl) {
        if(!s.isEmpty())
        {
            uint32_t td = s.toUInt(&ok, 16);
            int pos = ret.size();
            if (ok)
            {
                do {
                    ret.insert(pos, td & 0xFF);
//                    ret.append(td & 0xFF);
                    td >>= 8;
                }while(td > 0);
            }
        }
    }
    qDebug()<<ret;
    return ret;
}

// 客户端的数据接受函数，不用管
void MainWindow::ReadData()
{
    QByteArray buffer = tcpClient->readAll();
    if(!buffer.isEmpty())
    {
        ui->EditRecv->moveCursor(QTextCursor::End, QTextCursor::MoveAnchor);
//        ui->EditRecv->append(buffer);
        if (ui->chkHexDisp->isChecked())
        {
            QString tstr, t;
            for (int i = 0; i < buffer.size(); ++i)
            {
                t.sprintf("%02X ", buffer.data()[i]);
                tstr += t;
            }
            ui->EditRecv->insertPlainText(tstr);
        }
        else
            ui->EditRecv->insertPlainText(QString(buffer));
    }
}

void MainWindow::ReadError(QAbstractSocket::SocketError)
{
    tcpClient->disconnectFromHost();
    ui->BtnConn->setText(tr("连接服务器"));
    ui->EditRecv->append(tr("服务器连接错误：%1").arg(tcpClient->errorString()));
    ui->BtnSend->setEnabled(false);
}

void MainWindow::NewConnectionSlot()
{
    currentClient = tcpServer->nextPendingConnection();
    lstClient.append(currentClient);
    connect(currentClient, SIGNAL(readyRead()), this, SLOT(ServerReadData()));
    connect(currentClient, SIGNAL(disconnected()), this, SLOT(disconnectedSlot()));

    if (ui->cbLstClients->count() == 0)
        ui->cbLstClients->addItem("全部连接");
    ui->cbLstClients->addItem(currentClient->peerAddress().toString());
    if (ui->cbLstClients->count() > 0)
        ui->BtnSend->setEnabled(true);
}

void MainWindow::disconnectedSlot()
{
    for(int i = lstClient.length() - 1; i >= 0; --i)
    {
        if(lstClient[i]->state() == QAbstractSocket::UnconnectedState)
        {
            // 删除存储在combox中的客户端信息
            ui->cbLstClients->removeItem(ui->cbLstClients->findText(lstClient[i]->peerAddress().toString()));
            // 删除存储在tcpClient列表中的客户端信息
            lstClient[i]->destroyed();
            lstClient.removeAt(i);
        }
    }
    if (ui->cbLstClients->count() == 1)
    {
        ui->cbLstClients->clear();
        ui->BtnSend->setEnabled(false);
    }
}

void MainWindow::ServerReadData()
{
    // 由于readyRead信号并未提供SocketDecriptor，所以需要遍历所有客户端
    static QString IP_Port, IP_Port_Pre, frame;
    for(int i = 0; i < lstClient.length(); ++i)
    {
        QByteArray buffer = lstClient[i]->readAll();
        if(buffer.isEmpty())
            continue;
        /////////////////////在这里对读取的数据进行处理///////////////////////
        QString strRecv = QString(buffer);
        int pos;
        // 一次读取可能包含多个帧或者不完整帧
        while((pos = strRecv.indexOf('\n')) != -1) // 一帧以换行符结尾
        {
            frame += strRecv.left(pos);// 即从左往右pos个字符
            //完整数据帧处理
            frameProc(frame);
            frame.clear();
            //剩余数据
            strRecv = strRecv.right(strRecv.length()-pos-1); // 从右往左的strRecv.length()-pos-1个字符,不包含\n
        }
        frame += strRecv; // 非完整数据帧
        /////////////////////////////////////////////////////////////////


        // 以下部分是将数据放到接收框中
        ui->EditRecv->moveCursor(QTextCursor::End, QTextCursor::MoveAnchor);
        IP_Port = tr("[%1:%2]:").arg(lstClient[i]->peerAddress().toString()).arg(lstClient[i]->peerPort());

        // 若此次消息的地址与上次不同，则需显示此次消息的客户端地址
        if(IP_Port != IP_Port_Pre)
            ui->EditRecv->append(IP_Port);

        if (ui->chkHexDisp->isChecked())
        {
            QString tstr, t;
            for (int i = 0; i < buffer.size(); ++i)
            {
                t.sprintf("%02X ", buffer.data()[i]);
                tstr += t;
            }
            ui->EditRecv->insertPlainText(tstr);
        }
        else
            ui->EditRecv->insertPlainText(QString(buffer)+"\n");

        //更新ip_port
        IP_Port_Pre = IP_Port;
    }
}

void MainWindow::on_BtnConn_clicked()
{
    if (ui->radioClient->isChecked())
    {
        if (tcpClient->state() == QAbstractSocket::ConnectedState)
        {
            tcpClient->disconnectFromHost();    // 断开连接
            if (tcpClient->state() == QAbstractSocket::UnconnectedState || tcpClient->waitForDisconnected(1000))
            {
                ui->BtnConn->setText("连接服务器");
                ui->BtnSend->setEnabled(false);
            }
        }
        else
        {
            tcpClient->connectToHost(ui->EditIP->text(), ui->EditPort->text().toUShort());
            if (tcpClient->waitForConnected(1000))  // 连接成功则进入if{}
            {
                QFile file("conf.ini");
                file.open(QIODevice::WriteOnly | QIODevice::Text);
                file.write((ui->EditIP->text() + ":" + ui->EditPort->text()).toUtf8());
                file.close();
                ui->BtnConn->setText("断开连接");
                ui->BtnSend->setEnabled(true);
            }
            else
            {
                ui->EditRecv->append(tr("服务器连接错误：%1").arg(tcpClient->errorString()));
            }
        }
    }
    else {
        ui->cbLstClients->clear();
        if (tcpServer->isListening()) {
            for(int i = lstClient.length() - 1; i >= 0; --i) //断开所有连接
            {
                QTcpSocket *tt = lstClient.at(i);
                tt->disconnectFromHost();
                if (tt->state() == QAbstractSocket::UnconnectedState || tt->waitForDisconnected(1000))
                {
                 // 处理异常
                }
                lstClient.removeAt(i);  //从保存的客户端列表中取去除
            }
            tcpServer->close();     //不再监听端口
            ui->cbLstClients->clear();
            ui->BtnConn->setText("开始侦听");
            ui->BtnSend->setEnabled(false);
        }
        else {
            bool ok = tcpServer->listen(QHostAddress::AnyIPv4, ui->EditPort->text().toUShort());
            if(ok)
            {
                ui->BtnConn->setText("断开连接");
                ui->BtnSend->setEnabled(false);
            }
        }
    }
}

void MainWindow::on_BtnClearRecv_clicked()
{
    ui->EditRecv->clear();
}

void MainWindow::SendData(QByteArray& data)
{
    if (ui->radioClient->isChecked())
    {
        if(!data.isEmpty())
        {
            tcpClient->write(data);
        }
    }
    else {
        //全部连接
        if(ui->cbLstClients->currentIndex() == 0)
        {
            for(int i=0; i < lstClient.length(); i++)
                lstClient[i]->write(data);
        }
        else {
            QString clientIP = ui->cbLstClients->currentText();
            for(int i=0; i < lstClient.length(); i++)
            {
                if(lstClient[i]->peerAddress().toString() == clientIP)
                {
                    lstClient[i]->write(data);
                    return; //ip:port唯一，无需继续检索
                }
            }
        }
    }
}

void MainWindow::on_BtnSend_clicked()
{
    QString data = ui->EditSend->toPlainText();
    QByteArray tba;
    if (ui->chkHexSend->isChecked())
        tba = HexStringToByteArray(data);
    else
        tba = data.toLatin1();
    SendData(tba);
}

void MainWindow::handleTimeout()
{
    QString data = ui->EditSend->toPlainText();
    QByteArray tba;
    if (ui->chkHexSend->isChecked())
        tba = HexStringToByteArray(data);
    else
        tba = data.toLatin1();
    SendData(tba);
}

void MainWindow::on_radioClient_clicked()
{
    if (tcpClient->state() == QAbstractSocket::ConnectedState)
        return;

    // 服务器断开
    if (tcpServer->isListening())
    {
        for(int i = lstClient.size() - 1; i >= 0; --i) //断开所有连接
        {
            QTcpSocket *tt = lstClient.at(i);
            tt->disconnectFromHost();
            if (tt->state() == QAbstractSocket::UnconnectedState || tt->waitForDisconnected(1000))
            {

            }
            lstClient.removeAt(i);  //从保存的客户端列表中取去除
        }
        tcpServer->close();     //不再监听端口
    }
    ui->cbLstClients->clear();
    ui->cbLstClients->setVisible(false);
    ui->labelAddr->setText("服务器地址：");

    // 加载远程服务器地址、端口
    QFile file("conf.ini");
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    QByteArray t = file.readAll();
    QString str(t);
    file.close();
    if (!t.isEmpty())
    {
        QStringList lst = str.split(":");
        if (lst.size() == 2)
        {
            ui->EditIP->setText(lst[0]);
            ui->EditPort->setText(lst[1]);
        }
    }

    if (tcpClient->state() == QAbstractSocket::UnconnectedState)
    {
        ui->BtnConn->setText("连接服务器");
        ui->BtnSend->setEnabled(false);
    }
    else {
        ui->BtnConn->setText("断开连接");
        ui->BtnSend->setEnabled(true);
    }
}

void MainWindow::on_radioServer_clicked()
{
    if (tcpServer->isListening())
        return;

    // 断开客户端
    tcpClient->disconnectFromHost();    // 断开连接
    if (tcpClient->state() == QAbstractSocket::UnconnectedState || tcpClient->waitForDisconnected(1000))
    {

    }

    // 获取本地IP地址
    GetLocalIPAddress();
    ui->BtnConn->setText("开始侦听");
    ui->BtnSend->setEnabled(false);

    ui->cbLstClients->clear();
    ui->cbLstClients->setVisible(true);
    ui->labelAddr->setText("本机地址：");
}

void MainWindow::frameProc(const QString& frame)
{
    static QString warnings[4]={"正常","温度报警","震动报警","温度、震动报警"}; // 报警框显示信息
    QJsonParseError parse_error;
    QJsonDocument doc = QJsonDocument::fromJson(frame.toLatin1(), &parse_error); // JSON格式化
    if (!doc.isNull() && parse_error.error == QJsonParseError::NoError)
    {
        QJsonObject obj = doc.object();
        if (obj["type"].toInt() == FrameType::DATA_LOAD) // 帧类型：下位机数据上传
        {
            data_pack.ax = obj["ax"].toInt();
            data_pack.ay = obj["ay"].toInt();
            data_pack.az = obj["az"].toInt();
            data_pack.gx = obj["gx"].toInt();
            data_pack.gy = obj["gy"].toInt();
            data_pack.gz = obj["gz"].toInt();
            data_pack.warning = obj["warning"].toInt();
            data_pack.pitch = obj["pitch"].toDouble();
            data_pack.roll = obj["roll"].toDouble();
            data_pack.yaw = obj["yaw"].toDouble();
            data_pack.temp = obj["temp"].toDouble();

            myChart.addNewPoint("temp", data_pack.temp);
            myChart.addNewPoint("ax", data_pack.ax);
            myChart.addNewPoint("ay", data_pack.ay);
            myChart.addNewPoint("az", data_pack.az);
            myChart.addNewPoint("gx", data_pack.gx);
            myChart.addNewPoint("gy", data_pack.gy);
            myChart.addNewPoint("gz", data_pack.gz);
            myChart.addNewPoint("roll", data_pack.roll);
            myChart.addNewPoint("pitch", data_pack.pitch);
            myChart.addNewPoint("yaw", data_pack.yaw);
            myChart.updateFrame();


            ui->ax->setText(QString::number(data_pack.ax));
            ui->ay->setText(QString::number(data_pack.ay));
            ui->az->setText(QString::number(data_pack.az));
            ui->gx->setText(QString::number(data_pack.gx));
            ui->gy->setText(QString::number(data_pack.gy));
            ui->gz->setText(QString::number(data_pack.gz));
            ui->warning->setText(warnings[data_pack.warning]);
            ui->pitch->setText(QString::number(data_pack.pitch)+"°");
            ui->roll->setText(QString::number(data_pack.roll)+"°");
            ui->yaw->setText(QString::number(data_pack.yaw)+"°");
            ui->temp->setText(QString::number(data_pack.temp)+"℃");
        }
        else if (obj["type"].toInt() == FrameType::PARAM_LOAD) // 帧类型：下位机参数上传
        {
            param_pack.max_temp = obj["max_temp"].toInt();
            param_pack.shake_level = obj["shake_level"].toInt();
            param_pack.upload_time = obj["upload_time"].toDouble();
            param_pack.warning_time = obj["warning_time"].toInt();
            ui->max_temp->setValue(param_pack.max_temp);
            ui->shake_level->setCurrentIndex(param_pack.shake_level);
            ui->upload_time->setValue(param_pack.upload_time);
            ui->warning_time->setValue(param_pack.warning_time);
        }
        else
        {
            qWarning() << QString("帧类型错误");
        }
    }
    else
    {
        qWarning() << QString("帧格式错误");
    }
}

void MainWindow::on_load_param_clicked()
{
    QJsonObject obj;
    obj["type"] = FrameType::CMD_PARAM_LOAD;
    QJsonDocument doc(obj);
    QByteArray bytes = doc.toJson(QJsonDocument::Compact);
    SendData(bytes);
}

void MainWindow::on_update_param_clicked()
{
    QJsonObject obj;
    obj["type"] = FrameType::PARAM_SET;
    obj["max_temp"] = ui->max_temp->value();
    obj["shake_level"] = ui->shake_level->currentIndex();
    obj["upload_time"] = ui->upload_time->value();
    obj["warning_time"] = ui->warning_time->value();
    QJsonDocument doc(obj);
    QByteArray bytes = doc.toJson(QJsonDocument::Compact);
    SendData(bytes);
}

void MainWindow::on_uploadBtn_clicked()
{
    static bool status = false;
    status = !status;
    ui->uploadBtn->setText(status?QString("停止传输"):QString("开始传输"));
    QJsonObject obj;
    obj["type"] = FrameType::CMD_DATA_LOAD;
    obj["wifi_upload"] = status?1:0;
    QJsonDocument doc(obj);
    QByteArray bytes = doc.toJson(QJsonDocument::Compact);
    SendData(bytes);
}

void MainWindow::on_checkTemp_stateChanged(int arg1)
{
    arg1?myChart.enableSerie("temp"):myChart.disableSerie("temp");
}

void MainWindow::on_checkAx_stateChanged(int arg1)
{
    arg1?myChart.enableSerie("ax"):myChart.disableSerie("ax");
}

void MainWindow::on_checkAy_stateChanged(int arg1)
{
    arg1?myChart.enableSerie("ay"):myChart.disableSerie("ay");
}

void MainWindow::on_checkAz_stateChanged(int arg1)
{
    arg1?myChart.enableSerie("az"):myChart.disableSerie("az");
}

void MainWindow::on_checkGx_stateChanged(int arg1)
{
    arg1?myChart.enableSerie("gx"):myChart.disableSerie("gx");
}

void MainWindow::on_checkGy_stateChanged(int arg1)
{
    arg1?myChart.enableSerie("gy"):myChart.disableSerie("gy");
}

void MainWindow::on_checkGz_stateChanged(int arg1)
{
    arg1?myChart.enableSerie("gz"):myChart.disableSerie("gz");
}

void MainWindow::on_checkPitch_stateChanged(int arg1)
{
    arg1?myChart.enableSerie("pitch"):myChart.disableSerie("pitch");
}

void MainWindow::on_checkRoll_stateChanged(int arg1)
{
    arg1?myChart.enableSerie("roll"):myChart.disableSerie("roll");
}

void MainWindow::on_checkYaw_stateChanged(int arg1)
{
    arg1?myChart.enableSerie("yaw"):myChart.disableSerie("yaw");
}

void MainWindow::on_chkTimerSend_stateChanged(int arg1)
{
    arg1?m_timer.start():m_timer.stop();
}
