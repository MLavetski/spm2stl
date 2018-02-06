#include "converter.h"
#include <QApplication>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QStringList test = QStyleFactory::keys();
    QApplication a(argc, argv);
    a.setStyle("WindowsXP");
    converter w;
    w.show();

    return a.exec();
}
