#include "allowedappsdialog.h"
#include "appwhitelist.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFont>
#include <QPainter>
#include <QMouseEvent>
#include <QApplication>
#include <QScreen>

#ifdef Q_OS_WIN
#include <windows.h>
#include <tlhelp32.h>
#endif

// ─────────────────────────────────────────────────────────────────────────────
static QStringList runningProcesses()
{
    QStringList list;
#ifdef Q_OS_WIN
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return list;
    PROCESSENTRY32W pe{}; pe.dwSize = sizeof(pe);
    if (Process32FirstW(snap, &pe)) {
        do {
            QString name = QString::fromWCharArray(pe.szExeFile).toLower();
            if (!list.contains(name)) list.append(name);
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    list.sort();
#endif
    return list;
}

// ─────────────────────────────────────────────────────────────────────────────
static QString darkStyle()
{
    return R"(
        QDialog, QWidget {
            background: #0a1622; color: rgba(180,210,230,220);
        }
        QListWidget {
            background: #071018; border: 1px solid rgba(0,100,160,80);
            border-radius: 4px; outline: none;
            color: rgba(160,200,225,200);
        }
        QListWidget::item { padding: 4px 8px; }
        QListWidget::item:selected {
            background: rgba(0,140,200,60);
            color: rgba(0,220,255,240);
        }
        QListWidget::item:hover { background: rgba(0,100,160,30); }
        QLineEdit {
            background: #071018; border: 1px solid rgba(0,100,160,100);
            border-radius: 4px; padding: 5px 8px;
            color: rgba(180,220,240,220);
        }
        QLineEdit:focus { border-color: rgba(0,180,255,180); }
        QPushButton {
            background: rgba(0,100,160,30); color: rgba(0,195,255,220);
            border: 1px solid rgba(0,140,200,120); border-radius: 4px;
            padding: 5px 14px; font-family: "Courier New"; font-size: 11px;
            letter-spacing: 1px;
        }
        QPushButton:hover { background: rgba(0,140,200,70); border-color: rgba(0,200,255,200); }
        QPushButton:pressed { background: rgba(0,160,220,110); }
        QPushButton#okBtn {
            background: rgba(0,150,80,40); color: rgba(0,220,130,220);
            border-color: rgba(0,180,100,140);
        }
        QPushButton#okBtn:hover { background: rgba(0,180,100,80); }
        QPushButton#removeBtn {
            background: rgba(180,30,30,30); color: rgba(220,80,80,220);
            border-color: rgba(180,40,40,120);
        }
        QPushButton#removeBtn:hover { background: rgba(200,40,40,70); }
        QLabel { background: transparent; }
        QLabel#sectionLabel {
            color: rgba(0,180,220,180); font-family: "Courier New";
            font-size: 10px; letter-spacing: 2px;
        }
        QWidget#titleBar {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:0,
                stop:0 #060f1c, stop:0.5 #0a1828, stop:1 #060f1c);
            border-bottom: 1px solid rgba(0,130,190,80);
        }
        QLabel#titleText {
            color: rgba(0,210,255,200); font-family: "Courier New";
            font-size: 11px; letter-spacing: 3px;
            background: transparent;
        }
        QPushButton#closeBtn {
            background: transparent; color: rgba(100,160,200,160);
            border: none; font-size: 14px; padding: 0px;
            min-width: 32px; max-width: 32px;
        }
        QPushButton#closeBtn:hover { color: rgba(220,80,80,240); background: rgba(200,30,30,40); }
    )";
}

// ─────────────────────────────────────────────────────────────────────────────
AllowedAppsDialog::AllowedAppsDialog(QWidget *parent) : QDialog(parent)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, false);
    setMinimumSize(680, 460);
    setStyleSheet(darkStyle());

    m_working = AppWhitelistManager::instance().apps();

    // ── Outer layout (title bar + content) ───────────────────────
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    // ── Custom title bar ─────────────────────────────────────────
    auto *titleBar = new QWidget;
    titleBar->setObjectName("titleBar");
    titleBar->setFixedHeight(36);
    auto *tbLayout = new QHBoxLayout(titleBar);
    tbLayout->setContentsMargins(14, 0, 6, 0);
    tbLayout->setSpacing(8);

    // Cyan dot accent
    auto *dot = new QLabel("●");
    QFont dotF("Arial"); dotF.setPixelSize(9);
    dot->setFont(dotF);
    dot->setStyleSheet("color: rgba(0,200,255,180); background: transparent;");
    tbLayout->addWidget(dot);

    auto *titleText = new QLabel("ALLOWED APPS  —  FOCUS WHITELIST");
    titleText->setObjectName("titleText");
    QFont ttf("Courier New"); ttf.setPixelSize(11); ttf.setLetterSpacing(QFont::AbsoluteSpacing, 3);
    titleText->setFont(ttf);
    tbLayout->addWidget(titleText);
    tbLayout->addStretch();

    auto *closeBtn = new QPushButton("✕");
    closeBtn->setObjectName("closeBtn");
    closeBtn->setToolTip("Close");
    tbLayout->addWidget(closeBtn);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::reject);

    outer->addWidget(titleBar);

    // ── Content area ─────────────────────────────────────────────
    auto *content = new QWidget;
    auto *root    = new QVBoxLayout(content);
    root->setContentsMargins(18, 12, 18, 14);
    root->setSpacing(10);
    outer->addWidget(content, 1);

    QFont tf("Courier New"); tf.setPixelSize(10); tf.setLetterSpacing(QFont::AbsoluteSpacing, 2);

    // ── Two columns ───────────────────────────────────────────────
    auto *cols = new QHBoxLayout; cols->setSpacing(16);

    // Left: whitelist
    auto *leftCol = new QVBoxLayout; leftCol->setSpacing(6);
    auto *wlLabel = new QLabel("ALLOWED APPS");
    wlLabel->setObjectName("sectionLabel"); wlLabel->setFont(tf);
    leftCol->addWidget(wlLabel);

    m_appList = new QListWidget;
    m_appList->setSelectionMode(QAbstractItemView::SingleSelection);
    leftCol->addWidget(m_appList, 1);

    // Add manually row
    auto *addRow = new QHBoxLayout; addRow->setSpacing(6);
    m_addEdit = new QLineEdit;
    m_addEdit->setPlaceholderText("e.g. myapp.exe");
    m_addBtn  = new QPushButton("+ ADD");
    m_addBtn->setFixedWidth(72);
    addRow->addWidget(m_addEdit);
    addRow->addWidget(m_addBtn);
    leftCol->addLayout(addRow);

    m_removeBtn = new QPushButton("✕  REMOVE SELECTED");
    m_removeBtn->setObjectName("removeBtn");
    leftCol->addWidget(m_removeBtn);

    cols->addLayout(leftCol, 1);

    // Right: running processes
    auto *rightCol = new QVBoxLayout; rightCol->setSpacing(6);
    auto *rpLabel  = new QLabel("RUNNING PROCESSES  (click to add →)");
    rpLabel->setObjectName("sectionLabel"); rpLabel->setFont(tf);
    rightCol->addWidget(rpLabel);

    m_runningList = new QListWidget;
    m_runningList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_runningList->setToolTip("Double-click or press '+ ADD' to whitelist");
    rightCol->addWidget(m_runningList, 1);

    m_pickBtn = new QPushButton("←  ADD TO WHITELIST");
    rightCol->addWidget(m_pickBtn);

    auto *refreshBtn = new QPushButton("⟳  REFRESH LIST");
    rightCol->addWidget(refreshBtn);

    cols->addLayout(rightCol, 1);
    root->addLayout(cols, 1);

    // ── Bottom buttons ────────────────────────────────────────────
    auto *btnRow = new QHBoxLayout;
    btnRow->addStretch();
    m_cancelBtn = new QPushButton("CANCEL");
    m_okBtn     = new QPushButton("✓  SAVE");
    m_okBtn->setObjectName("okBtn");
    m_okBtn->setDefault(true);
    btnRow->addWidget(m_cancelBtn);
    btnRow->addWidget(m_okBtn);
    root->addLayout(btnRow);

    // ── Populate ──────────────────────────────────────────────────
    refreshList();
    refreshRunning();

    // ── Connections ───────────────────────────────────────────────
    connect(m_addBtn,     &QPushButton::clicked, this, &AllowedAppsDialog::onAdd);
    connect(m_addEdit,    &QLineEdit::returnPressed, this, &AllowedAppsDialog::onAdd);
    connect(m_removeBtn,  &QPushButton::clicked, this, &AllowedAppsDialog::onRemove);
    connect(m_pickBtn,    &QPushButton::clicked, this, &AllowedAppsDialog::onPickRunning);
    connect(m_okBtn,      &QPushButton::clicked, this, &AllowedAppsDialog::onAccept);
    connect(m_cancelBtn,  &QPushButton::clicked, this, &QDialog::reject);
    connect(refreshBtn,   &QPushButton::clicked, this, &AllowedAppsDialog::refreshRunning);
    connect(m_runningList,&QListWidget::itemDoubleClicked, this, &AllowedAppsDialog::onPickRunning);
}

// ─────────────────────────────────────────────────────────────────────────────
void AllowedAppsDialog::refreshList()
{
    m_appList->clear();
    for (const auto &a : m_working)
        m_appList->addItem(a);
}

void AllowedAppsDialog::refreshRunning()
{
    m_runningList->clear();
    for (const auto &p : runningProcesses())
        m_runningList->addItem(p);
}

void AllowedAppsDialog::onAdd()
{
    QString name = m_addEdit->text().trimmed().toLower();
    if (name.isEmpty()) return;
    if (!name.endsWith(".exe")) name += ".exe";
    if (!m_working.contains(name)) {
        m_working.append(name);
        refreshList();
    }
    m_addEdit->clear();
}

void AllowedAppsDialog::onRemove()
{
    auto *item = m_appList->currentItem();
    if (!item) return;
    m_working.removeAll(item->text());
    refreshList();
}

void AllowedAppsDialog::onPickRunning()
{
    auto *item = m_runningList->currentItem();
    if (!item) return;
    QString name = item->text().toLower();
    if (!m_working.contains(name)) {
        m_working.append(name);
        refreshList();
    }
}

void AllowedAppsDialog::onAccept()
{
    AppWhitelistManager::instance().setApps(m_working);
    accept();
}

// ─────────────────────────────────────────────────────────────────────────────
// Drag-to-move (frameless window)
void AllowedAppsDialog::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton && e->pos().y() < 36) {
        m_dragging    = true;
        m_dragOffset  = e->globalPosition().toPoint() - frameGeometry().topLeft();
        e->accept();
    } else {
        QDialog::mousePressEvent(e);
    }
}

void AllowedAppsDialog::mouseMoveEvent(QMouseEvent *e)
{
    if (m_dragging)
        move(e->globalPosition().toPoint() - m_dragOffset);
    else
        QDialog::mouseMoveEvent(e);
}

void AllowedAppsDialog::mouseReleaseEvent(QMouseEvent *e)
{
    m_dragging = false;
    QDialog::mouseReleaseEvent(e);
}
