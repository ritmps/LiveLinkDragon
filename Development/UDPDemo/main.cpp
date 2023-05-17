#include <QApplication>

#include "jsontest.h"
#include <QTimer>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    JsonTest * t = new JsonTest();

    QTimer::singleShot(10, t, SLOT(show()));

    return a.exec();
}
