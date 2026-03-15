#pragma once
#include <QMainWindow>
#include <QShowEvent>

class DashboardWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void showEvent  (QShowEvent  *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void applyDarkTitleBar();
    DashboardWidget *m_dashboard{nullptr};
};
