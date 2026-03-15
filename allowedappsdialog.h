#pragma once
#include <QDialog>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QMouseEvent>

class AllowedAppsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AllowedAppsDialog(QWidget *parent = nullptr);

protected:
    void mousePressEvent  (QMouseEvent *e) override;
    void mouseMoveEvent   (QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;

private slots:
    void onAdd();
    void onRemove();
    void onPickRunning();
    void onAccept();

private:
    void refreshList();
    void refreshRunning();

    QListWidget *m_appList    {nullptr};
    QListWidget *m_runningList{nullptr};
    QLineEdit   *m_addEdit    {nullptr};
    QPushButton *m_addBtn     {nullptr};
    QPushButton *m_removeBtn  {nullptr};
    QPushButton *m_pickBtn    {nullptr};
    QPushButton *m_okBtn      {nullptr};
    QPushButton *m_cancelBtn  {nullptr};

    QStringList m_working;

    // Drag-to-move (frameless window)
    bool    m_dragging{false};
    QPoint  m_dragOffset;
};
