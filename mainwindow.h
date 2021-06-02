#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>
#include <QTcpServer>
#include <QHostAddress>
#include <QNetworkInterface>
#include <QtCore>
#include <QtCharts/QChartView>
#include "chart.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void GetLocalIPAddress();
    QByteArray HexStringToByteArray(QString HexString);

private:
    Ui::MainWindow *ui;
    QTcpSocket *tcpClient;
    QTcpServer *tcpServer;
    QList<QTcpSocket*> lstClient;
    QTcpSocket *currentClient;
    Chart myChart;
    QTimer m_timer;
    struct DataPack
    {
        int ax;
        int ay;
        int az;
        int gx;
        int gy;
        int gz;
        int warning;
        float pitch;
        float yaw;
        float roll;
        float temp;
    };
    struct ParamPack
    {
       int max_temp;
       int shake_level;
       int warning_time;
       float upload_time;
    };
    enum FrameType
    {
        DATA_LOAD = 0,
        PARAM_LOAD = 1,
        CMD_PARAM_LOAD =2,
        CMD_DATA_LOAD = 3,
        PARAM_SET = 4
    };

    DataPack data_pack;
    ParamPack param_pack;
    void frameProc(const QString& frame);
    void SendData(QByteArray& data);

private slots://槽函数
    void ReadData();
    void ReadError(QAbstractSocket::SocketError);

    void NewConnectionSlot();
    void disconnectedSlot();
    void ServerReadData();

    void on_BtnConn_clicked();
    void on_BtnClearRecv_clicked();
    void on_BtnSend_clicked();
    void on_radioClient_clicked();
    void on_radioServer_clicked();
    void on_load_param_clicked();
    void on_update_param_clicked();
    void on_uploadBtn_clicked();
    void on_checkTemp_stateChanged(int arg1);
    void on_checkAx_stateChanged(int arg1);
    void on_checkRoll_stateChanged(int arg1);
    void on_checkAy_stateChanged(int arg1);
    void on_checkAz_stateChanged(int arg1);
    void on_checkGx_stateChanged(int arg1);
    void on_checkGy_stateChanged(int arg1);
    void on_checkGz_stateChanged(int arg1);
    void on_checkPitch_stateChanged(int arg1);
    void on_checkYaw_stateChanged(int arg1);

    void handleTimeout();
    void on_chkTimerSend_stateChanged(int arg1);
    void on_chartScrollBar_sliderMoved(int position);
};

#endif // MAINWINDOW_H
