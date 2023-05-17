#include "jsontest.h"

#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDebug>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

JsonTest::JsonTest(QWidget *parent) :
    QWidget(parent),
    dfHostPort(0)
{
    setWindowTitle("Dragonframe JSON Tester");

    setMinimumSize(300, 500);

    QVBoxLayout * layout = new QVBoxLayout();
    setLayout(layout);

    QPushButton * shootOneButton = new QPushButton("Shoot");
    layout->addWidget(shootOneButton);
    connect(shootOneButton, SIGNAL(pressed()), this, SLOT(shootOne()));

    QPushButton * shootThreeButton = new QPushButton("Shoot 3");
    layout->addWidget(shootThreeButton);
    connect(shootThreeButton, SIGNAL(pressed()), this, SLOT(shootThree()));

    QPushButton * shootVideoAssistButton = new QPushButton("Shoot Video Assist");
    layout->addWidget(shootVideoAssistButton);
    connect(shootVideoAssistButton, SIGNAL(pressed()), this, SLOT(shootVideoAssistOnly()));

    QPushButton * playButton = new QPushButton("Play/Pause");
    layout->addWidget(playButton);
    connect(playButton, SIGNAL(pressed()), this, SLOT(playPause()));

    QPushButton * deleteFrameButton = new QPushButton("Delete");
    layout->addWidget(deleteFrameButton);
    connect(deleteFrameButton, SIGNAL(pressed()), this, SLOT(deleteFrame()));

    QPushButton * liveButton = new QPushButton("Go to Live");
    layout->addWidget(liveButton);
    connect(liveButton, SIGNAL(pressed()), this, SLOT(live()));

    QPushButton * muteButton = new QPushButton("Mute");
    layout->addWidget(muteButton);
    connect(muteButton, SIGNAL(pressed()), this, SLOT(mute()));

    QPushButton * blackButton = new QPushButton("Black Frames");
    layout->addWidget(blackButton);
    connect(blackButton, SIGNAL(pressed()), this, SLOT(black()));

    QPushButton * loopButton = new QPushButton("Loop");
    layout->addWidget(loopButton);
    connect(loopButton, SIGNAL(pressed()), this, SLOT(loop()));

    QLayout * opacityButtons = new QHBoxLayout;

    QPushButton * opacityDownButton = new QPushButton("Opacity Down");
    opacityButtons->addWidget(opacityDownButton);
    connect(opacityDownButton, SIGNAL(pressed()), this, SLOT(opacityDown()));

    QPushButton * opacityUpButton = new QPushButton("Opacity Up");
    opacityButtons->addWidget(opacityUpButton);
    connect(opacityUpButton, SIGNAL(pressed()), this, SLOT(opacityUp()));

    layout->addLayout(opacityButtons);

    QLayout * stepButtons = new QHBoxLayout;

    QPushButton * stepBackwardButton = new QPushButton("<");
    stepButtons->addWidget(stepBackwardButton);
    connect(stepBackwardButton, SIGNAL(pressed()), this, SLOT(stepBackward()));

    QPushButton * stepForwardButton = new QPushButton(">");
    stepButtons->addWidget(stepForwardButton);
    connect(stepForwardButton, SIGNAL(pressed()), this, SLOT(stepForward()));

    layout->addLayout(stepButtons);


    QLayout * liveToggleButtons = new QHBoxLayout;

    QPushButton * liveTogglePressedButton = new QPushButton("Live Toggle PRESS");
    liveToggleButtons->addWidget(liveTogglePressedButton);
    connect(liveTogglePressedButton, SIGNAL(pressed()), this, SLOT(liveTogglePressed()));

    QPushButton * liveToggleReleasedButton = new QPushButton("Live Toggle RELEASE");
    liveToggleButtons->addWidget(liveToggleReleasedButton);
    connect(liveToggleReleasedButton, SIGNAL(pressed()), this, SLOT(liveToggleReleased()));

    layout->addLayout(liveToggleButtons);


    QPushButton * toggleButton = new QPushButton("Toggle");
    layout->addWidget(toggleButton);
    connect(toggleButton, SIGNAL(pressed()), this, SLOT(autoToggle()));

    QPushButton * hiResButton = new QPushButton("Hi-Res");
    layout->addWidget(hiResButton);
    connect(hiResButton, SIGNAL(pressed()), this, SLOT(highResToggle()));

    QPushButton * getSceneInfoButton = new QPushButton("Get Scene Info");
    layout->addWidget(getSceneInfoButton);
    connect(getSceneInfoButton, SIGNAL(pressed()), this, SLOT(getSceneInfo()));


    layout->addStretch();

    start();
}

void JsonTest::start()
{
    qDebug() << "Starting JsonTest" << socket.bind(8888);

    connect(&socket, SIGNAL(readyRead()), this, SLOT(readPendingDatagrams()));
}


void JsonTest::readPendingDatagrams()
{
    while (socket.hasPendingDatagrams())
    {
        QByteArray datagram;
        datagram.resize(socket.pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort = 0;

        socket.readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

        if (sender != dfHostAddress || senderPort != dfHostPort)
        {
            dfHostAddress = sender;
            dfHostPort = senderPort;
            qDebug() << "New Dragonframe host" << sender << senderPort;
        }

        QJsonDocument doc = QJsonDocument::fromJson(datagram);
        qDebug() << "Event" << QString(doc.toJson());

        QJsonObject obj = doc.object();
        if (!obj.isEmpty() && obj["event"].isString())
        {
            receiveEvent(obj);
        }

    }
}

void JsonTest::receiveEvent(const QJsonObject& obj)
{
    QString event = obj["event"].toString();

    if (event == "hello") // must respond to these
    {
        qDebug() << obj["minVersion"].toDouble()
                 << obj["maxVersion"].toDouble();

        QJsonObject json;
        json["command"] = QString("hello");
        json["version"] = 1.0;
        sendJson(json);
    }
    else if (event == "shoot")
    {
        qDebug() << "SHOOT FRAME"
                << obj["production"].toString()
                << obj["scene"].toString()
                << obj["take"].toString()
                << obj["frame"].toInt()
                << obj["exposure"].toInt()
                << obj["exposureName"].toString()
                << obj["stereoIndex"].toInt();
    }
    else if (event == "delete")
    {
        qDebug() << "DELETE"
                << obj["production"].toString()
                << obj["scene"].toString()
                << obj["take"].toString();
    }
    else if (event == "position")
    {
        qDebug() << "POSITION FRAME"
                << obj["production"].toString()
                << obj["scene"].toString()
                << obj["take"].toString()
                << obj["frame"].toInt()
                << obj["mocoFrame"].toInt()
                << obj["exposure"].toInt()
                << obj["exposureName"].toString()
                << obj["stereoIndex"].toInt();
    }
    else if (event == "captureComplete")
    {
        qDebug() << "CAPTURE COMPLETE"
                << obj["production"].toString()
                << obj["scene"].toString()
                << obj["take"].toString()
                << obj["frame"].toInt()
                << obj["exposure"].toInt()
                << obj["exposureName"].toString()
                << obj["stereoIndex"].toInt()
                << obj["imageFileName"].toString();
    }
    else if (event == "frameComplete")
    {
        qDebug() << "FRAME COMPLETE"
                << obj["production"].toString()
                << obj["scene"].toString()
                << obj["take"].toString()
                << obj["frame"].toInt()
                << obj["exposure"].toInt()
                << obj["exposureName"].toString()
                << obj["stereoIndex"].toInt()
                << obj["imageFileName"].toString();
    }
}


void JsonTest::sendJson(const QJsonObject& obj)
{
    if (dfHostPort)
    {
        QJsonDocument doc(obj);
        qDebug() << "SENDING " << QString(doc.toJson()) << " to " << dfHostAddress << dfHostPort;
        socket.writeDatagram(doc.toJson(), dfHostAddress, dfHostPort);
    }
    else
    {
        qDebug("Can't send message yet.");
    }
}


void JsonTest::shootOne()
{
    QJsonObject json;
    json["command"] = QString("shoot");
    json["frames"] = 1;
    sendJson(json);
}

void JsonTest::shootThree()
{
    QJsonObject json;
    json["command"] = QString("shoot");
    json["frames"] = 3;
    sendJson(json);
}

void JsonTest::shootVideoAssistOnly()
{
    QJsonObject json;
    json["command"] = QString("shootVideoAssist");
    sendJson(json);
}


void JsonTest::playPause()
{
    QJsonObject json;
    json["command"] = QString("play");
    sendJson(json);
}

void JsonTest::deleteFrame()
{
    QJsonObject json;
    json["command"] = QString("delete");
    sendJson(json);
}
void JsonTest::live()
{
    QJsonObject json;
    json["command"] = QString("live");
    sendJson(json);
}
void JsonTest::mute()
{
    QJsonObject json;
    json["command"] = QString("mute");
    sendJson(json);
}

void JsonTest::black()
{
    QJsonObject json;
    json["command"] = QString("black");
    sendJson(json);
}
void JsonTest::loop()
{
    QJsonObject json;
    json["command"] = QString("loop");
    sendJson(json);
}
void JsonTest::opacityDown()
{
    QJsonObject json;
    json["command"] = QString("opacityDown");
    sendJson(json);
}

void JsonTest::opacityUp()
{
    QJsonObject json;
    json["command"] = QString("opacityUp");
    sendJson(json);
}
void JsonTest::stepForward()
{
    QJsonObject json;
    json["command"] = QString("stepForward");
    sendJson(json);
}
void JsonTest::stepBackward()
{
    QJsonObject json;
    json["command"] = QString("stepBackward");
    sendJson(json);
}
void JsonTest::liveTogglePressed()
{
    QJsonObject json;
    json["command"] = QString("liveToggle");
    json["state"] = QString("pressed");
    sendJson(json);
}
void JsonTest::liveToggleReleased()
{
    QJsonObject json;
    json["command"] = QString("liveToggle");
    json["state"] = QString("released");
    sendJson(json);
}
void JsonTest::autoToggle()
{
    QJsonObject json;
    json["command"] = QString("autoToggle");
    sendJson(json);
}
void JsonTest::highResToggle()
{
    QJsonObject json;
    json["command"] = QString("highResToggle");
    sendJson(json);
}

void JsonTest::getSceneInfo()
{
    QJsonObject json;
    json["command"] = QString("getSceneInfo");
    sendJson(json);
}
