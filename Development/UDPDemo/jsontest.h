#ifndef JSONTEST_H
#define JSONTEST_H

#include <QWidget>
#include <QUdpSocket>
#include <QJsonObject>

class JsonTest : public QWidget
{
    Q_OBJECT
public:
    explicit JsonTest(QWidget *parent = 0);

signals:

public slots:
    void start();
    void readPendingDatagrams();

    void shootOne();
    void shootThree();
    void shootVideoAssistOnly();
    void playPause();
    void deleteFrame();
    void live();
    void mute();
    void black();
    void loop();
    void opacityDown();
    void opacityUp();
    void stepForward();
    void stepBackward();
    void liveTogglePressed();
    void liveToggleReleased();
    void autoToggle();
    void highResToggle();
    void getSceneInfo();

protected:
    void sendJson(const QJsonObject& obj);

    void receiveEvent(const QJsonObject& obj);

    QUdpSocket socket;

    QHostAddress dfHostAddress;
    int          dfHostPort;
};

#endif // JSONTEST_H
