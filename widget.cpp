#include "widget.h"
#include "ui_widget.h"
#include <QDebug>
#include <QString>
#include <QTimer>
#include <QtConcurrent/QtConcurrent>

Widget::Widget(const AppConfig& cfg, QWidget* parent) : QWidget(parent), ui(new Ui::Widget), config(cfg) {
    ui->setupUi(this);

    connect(this, SIGNAL(transmit_msg(QString)), this, SLOT(display(QString)));

    thread = new QThread;
    nsa = new NsaTransporter;
    nsa->setRuntimeConfig(config.lslName, config.lslType, config.lslSourceId, config.pingIntervalMs);
    nsa->moveToThread(thread);
    connect(thread, SIGNAL(started()), nsa, SLOT(init()));
    connect(thread, SIGNAL(started()), nsa, SLOT(apply_runtime_config()));
    connect(thread, SIGNAL(finished()), nsa, SLOT(deleteLater()));
    connect(this, SIGNAL(set_host(QString, int)), nsa, SLOT(set_host(QString, int)));
    connect(this, SIGNAL(send_code(QByteArray)), nsa, SLOT(send(QByteArray)));
    thread->start();

    ui->ipEdit->setText(config.host);
    ui->portEdit->setValue(config.port);

    if (config.autoconnect)
        QTimer::singleShot(0, this, SLOT(startSessionFromConfig()));
}

Widget::~Widget() {
    thread->quit();
    thread->wait();
    delete ui;
}

void Widget::displayMsg(QString str) { emit transmit_msg(str); }

void Widget::display(QString str) {
    // QMutexLocker locker(&mutex);
    ui->statusbar->append(str);
    // ui->statusbar->moveCursor(QTextCursor::End);
}

void Widget::startSessionFromConfig() { on_pushButton_clicked(); }

void Widget::on_pushButton_clicked() {

    disconnect(ui->pushButton, SIGNAL(clicked(bool)), this, SLOT(on_pushButton_clicked()));
    connect(ui->pushButton, SIGNAL(clicked(bool)), this, SLOT(thread_stop()));
    emit set_host(ui->ipEdit->text(), ui->portEdit->value());
    ui->pushButton->setText("disconnect");
    emit send_code(NsaPacket(HeaderIdCtrl, ClientControlCode, RequestBasicInfo).serialize());
    QThread::msleep(100);
    emit send_code(NsaPacket(HeaderIdCtrl, ServerControlCode, StartAcquisition).serialize());
    emit send_code(NsaPacket(HeaderIdCtrl, ClientControlCode, RequestStartData).serialize());

    return;
}
void Widget::thread_stop() {
    emit send_code(NsaPacket(HeaderIdCtrl, ClientControlCode, RequestStopData).serialize());
    emit send_code(NsaPacket(HeaderIdCtrl, GeneralControlCode, ClosingUp).serialize());
    QThread::msleep(100);
    emit set_host("", 0);
    disconnect(ui->pushButton, SIGNAL(clicked(bool)), this, SLOT(thread_stop()));
    connect(ui->pushButton, SIGNAL(clicked(bool)), this, SLOT(on_pushButton_clicked()));
    ui->pushButton->setText("connect");
}
