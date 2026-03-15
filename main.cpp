#include "mainwindow.h"
#include <QApplication>
#include <QIcon>
#include <QSurfaceFormat>
#include <QFont>

int main(int argc, char *argv[])
{
    // Enable high-DPI scaling
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    QApplication app(argc, argv);
    app.setApplicationName("Focus Driving Simulator");
    app.setOrganizationName("FocusApp");
    // Icon embedded via RC_ICONS in .pro (Windows exe icon + taskbar)
    // Also set programmatically for runtime window icon
    app.setWindowIcon(QIcon("app.ico"));

    // Global font
    QFont appFont("Segoe UI", 10);
    app.setFont(appFont);

    MainWindow w;
    w.show();

    return app.exec();
}
