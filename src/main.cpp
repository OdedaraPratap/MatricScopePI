#include "AutoWindow.h"

#include <QApplication>
#include <QFont>

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);
    application.setApplicationName("Matric Scope");
    application.setOrganizationName("MatricScopePI");

    QFont font = application.font();
    font.setPointSize(11);
    application.setFont(font);

    AutoWindow window;
    window.showFullScreen();
    return application.exec();
}
