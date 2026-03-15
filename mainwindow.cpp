#include "mainwindow.h"
#include "dashboardwidget.h"
#include <QResizeEvent>

#ifdef Q_OS_WIN
#include <windows.h>
#include <dwmapi.h>
#endif

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Focus Driving Simulator");
    setMinimumSize(900, 680);
    resize(1100, 760);

    m_dashboard = new DashboardWidget(this);
    setCentralWidget(m_dashboard);

    setStyleSheet("QMainWindow { background: #060C16; }");
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    applyDarkTitleBar();
}

void MainWindow::applyDarkTitleBar()
{
#ifdef Q_OS_WIN
    HWND hwnd = reinterpret_cast<HWND>(winId());

    // Works on Windows 10 build 19041+ and Windows 11
    // DWMWA_USE_IMMERSIVE_DARK_MODE = 20
    BOOL dark = TRUE;
    HRESULT hr = DwmSetWindowAttribute(hwnd,
        20 /* DWMWA_USE_IMMERSIVE_DARK_MODE */,
        &dark, sizeof(dark));

    if (FAILED(hr)) {
        // Fallback for older Windows 10 (pre-20H1): attribute index 19
        DwmSetWindowAttribute(hwnd, 19, &dark, sizeof(dark));
    }

    // Optional: accent border color — cyan to match dashboard theme
    // DWMWA_BORDER_COLOR = 34  (Windows 11 only, silently fails on Win10)
    COLORREF borderColor = 0x00C87800; // BGR: cyan #0078C8
    DwmSetWindowAttribute(hwnd, 34, &borderColor, sizeof(borderColor));
#endif
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
}
