#ifndef WIDGET_H
#define WIDGET_H

#include "NeuroscanAcquireProtocol.h"
#include "nsatransporter.h"
#include <QMutex>
#include <QMutexLocker>
#include <QTcpSocket>
#include <QWidget>

namespace Ui {
class Widget;
}

struct AppConfig {
    QString host;
    int port = 4000;
    bool autoconnect = false;
    QString lslName = "Neuroscan";
    QString lslType = "EEG";
    QString lslSourceId = "Neuroscan_sdetsff";
    int pingIntervalMs = 100;
};

class Widget : public QWidget {
    Q_OBJECT

public:
    explicit Widget(const AppConfig& cfg, QWidget* parent = 0);
    ~Widget();

public:
    void displayMsg(QString);

private slots:
    void on_pushButton_clicked();
    void thread_stop();
    void startSessionFromConfig();
    void display(QString);

signals:
    void set_host(const QString&, const int&);
    void transmit_msg(QString);
    void send_code(QByteArray);

private:
    Ui::Widget* ui;
    AppConfig config;

private:
    QThread* thread;
    NsaTransporter* nsa;

    //QMutex mutex;
};

#endif // WIDGET_H
