#include "dashboardwidget.h"
#include "focusengine.h"
#include "speedgaugewidget.h"
#include "gearwidget.h"
#include "mapwidget.h"
#include "allowedappsdialog.h"
#include <QDateTime>
#include <algorithm>
#include "rpmbarwidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QLinearGradient>
#include <QRadialGradient>
#include <QSizePolicy>

DashboardWidget::DashboardWidget(QWidget *parent) : QWidget(parent)
{
    m_engine = new FocusEngine(this);
    setupUI();
    setupConnections();

    m_flashAnim = new QPropertyAnimation(this, "flashAlpha", this);
    m_flashAnim->setDuration(600);
    m_flashAnim->setStartValue(0.7);
    m_flashAnim->setEndValue(0.0);
    m_flashAnim->setEasingCurve(QEasingCurve::OutQuad);

    m_warnAnim = new QPropertyAnimation(this, "warnAlpha", this);
    m_warnAnim->setDuration(800);
    m_warnAnim->setStartValue(0.8);
    m_warnAnim->setEndValue(0.0);
    m_warnAnim->setEasingCurve(QEasingCurve::OutQuad);

    m_hudTimer = new QTimer(this);
    m_hudTimer->setInterval(1000);
    connect(m_hudTimer, &QTimer::timeout, this, &DashboardWidget::updateHUD);
    m_hudTimer->start();
}

// ─────────────────────────────────────────────────────────────────────────────
void DashboardWidget::setupUI()
{
    setMinimumSize(860, 640);
    setStyleSheet("QWidget { background: transparent; color: white; }");

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(18, 14, 18, 14);
    root->setSpacing(6);

    // ── Top HUD bar ───────────────────────────────────────────────
    auto *topBar = new QHBoxLayout;

    m_timerLabel = new EtaWidget(this);
    m_timerLabel->setMinimumWidth(200);

    m_statusLabel = new QLabel("READY");
    QFont statusF; statusF.setPixelSize(12); statusF.setLetterSpacing(QFont::AbsoluteSpacing, 3);
    m_statusLabel->setFont(statusF);
    m_statusLabel->setStyleSheet("color: rgba(80,140,185,200); background: transparent;");
    m_statusLabel->setAlignment(Qt::AlignCenter);

    m_statsLabel = new QLabel("");
    QFont statsF; statsF.setPixelSize(11);
    m_statsLabel->setFont(statsF);
    m_statsLabel->setStyleSheet("color: rgba(70,115,155,170); background: transparent;");
    m_statsLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    topBar->addWidget(m_timerLabel, 1);
    topBar->addWidget(m_statusLabel, 2);
    topBar->addWidget(m_statsLabel, 1);
    root->addLayout(topBar);

    auto *sep = new QWidget; sep->setFixedHeight(1);
    sep->setStyleSheet("background: rgba(0,120,180,50);");
    root->addWidget(sep);

    // ── Gauges + RPM arc between them ────────────────────────────
    // Layout: [speedGauge] [rpmArc] [gearWidget]  — RPM tacho in the middle
    auto *gaugesRow = new QHBoxLayout; gaugesRow->setSpacing(8);
    m_speedGauge = new SpeedGaugeWidget(this);
    m_speedGauge->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_gearWidget = new GearWidget(this);
    m_gearWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_rpmBar = new RpmBarWidget(this);
    m_rpmBar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_rpmBar->setMinimumSize(180, 180);

    gaugesRow->addWidget(m_speedGauge, 3);
    gaugesRow->addWidget(m_rpmBar, 2);
    gaugesRow->addWidget(m_gearWidget, 3);
    root->addLayout(gaugesRow, 3);

    // ── Map ───────────────────────────────────────────────────────
    m_mapWidget = new MapWidget(this);
    m_mapWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_mapWidget->setMinimumHeight(145);
    root->addWidget(m_mapWidget, 2);

    // ── Controls ─────────────────────────────────────────────────
    auto *ctrlRow = new QHBoxLayout; ctrlRow->setSpacing(12);

    m_startCancelBtn = new QPushButton("START");
    m_resetBtn       = new QPushButton("RESET");
    m_muteBtn        = new QPushButton("");

    styleButton(m_startCancelBtn, QColor(0, 185, 90));
    styleButton(m_resetBtn,       QColor(190, 45, 45));
    styleButton(m_muteBtn,        QColor(80, 130, 180));

    // Segoe MDL2 icons: E768=Play E769=Pause E72C=Refresh E74F=Mute E767=Sound
    setButtonIcon(m_startCancelBtn, QString(QChar(0xE768)), QColor(0, 185, 90));
    setButtonIcon(m_resetBtn,       QString(QChar(0xE72C)), QColor(190, 45, 45));
    setButtonIcon(m_muteBtn,        QString(QChar(0xE767)), QColor(80, 130, 180));
    m_muteBtn->setMinimumSize(44, 40);
    m_muteBtn->setMaximumWidth(50);
    m_muteBtn->setToolTip("Toggle engine sound");


    // ── Strict mode toggle + allowed apps button ─────────────────
    m_appsBtn = new QPushButton("ALLOWED APPS");
    styleButton(m_appsBtn, QColor(70, 100, 140));
    m_appsBtn->setToolTip("Edit list of apps that count as focused work");
    setButtonIcon(m_appsBtn, QString(QChar(0xE713)), QColor(70, 100, 140));  // Settings gear

    m_historyBtn = new QPushButton("HISTORY");
    styleButton(m_historyBtn, QColor(60, 90, 140));
    m_historyBtn->setToolTip("View session history");
    setButtonIcon(m_historyBtn, QString(QChar(0xE81C)), QColor(60, 90, 140));  // History clock

    ctrlRow->addStretch();
    ctrlRow->addWidget(m_startCancelBtn);
    ctrlRow->addWidget(m_resetBtn);
    ctrlRow->addWidget(m_muteBtn);
    ctrlRow->addSpacing(12);
    ctrlRow->addWidget(m_appsBtn);
    ctrlRow->addWidget(m_historyBtn);
    ctrlRow->addStretch();
    root->addLayout(ctrlRow);
}

// Draw a QIcon from a Segoe MDL2 glyph at given color
static QIcon glyphIcon(const QString &glyph, const QColor &col, int sz = 18)
{
    QPixmap pix(sz, sz);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::TextAntialiasing);
    QFont f("Segoe MDL2 Assets", 1);
    f.setPixelSize(sz - 2);
    p.setFont(f);
    p.setPen(col);
    p.drawText(QRect(0, 0, sz, sz), Qt::AlignCenter, glyph);
    return QIcon(pix);
}

void DashboardWidget::styleButton(QPushButton *btn, const QColor &accent)
{
    QFont f; f.setFamily("Courier New"); f.setPixelSize(12);
    f.setBold(true); f.setLetterSpacing(QFont::AbsoluteSpacing, 3);
    btn->setFont(f);
    btn->setMinimumSize(138, 40);
    btn->setCursor(Qt::PointingHandCursor);
    btn->setIconSize(QSize(16, 16));
    btn->setStyleSheet(QString(R"(
        QPushButton {
            background: rgba(%1,%2,%3,22); color: rgba(%1,%2,%3,240);
            border: 1px solid rgba(%1,%2,%3,140); border-radius: 5px;
            padding: 6px 18px 6px 14px;
            text-align: center;
        }
        QPushButton:hover {
            background: rgba(%1,%2,%3,55);
            border: 1px solid rgba(%1,%2,%3,220);
        }
        QPushButton:pressed { background: rgba(%1,%2,%3,100); }
        QPushButton:disabled {
            background: rgba(30,45,65,40); color: rgba(80,110,140,100);
            border: 1px solid rgba(50,75,100,80);
        }
    )").arg(accent.red()).arg(accent.green()).arg(accent.blue()));
}

// Set button icon glyph (call after styleButton so color is known)
void DashboardWidget::setButtonIcon(QPushButton *btn, const QString &glyph, const QColor &col)
{
    btn->setIcon(glyphIcon(glyph, col));
}

// ─────────────────────────────────────────────────────────────────────────────
static SessionRecord buildRecord(FocusEngine *e, const QString &outcome) {
    SessionRecord r;
    r.startTime      = e->startTime();
    r.endTime        = QDateTime::currentDateTime();
    r.elapsedSeconds = e->focusTimeSeconds();
    r.goalSeconds    = e->sessionGoal();
    r.maxSpeed       = e->maxSpeed();
    r.maxRpm         = e->maxRpm();
    r.peakGear       = e->peakGear();
    r.avgSpeed       = e->avgSpeed();
    r.completionPct  = std::min(100.0, r.elapsedSeconds * 100.0 / std::max(1, r.goalSeconds));
    r.distractions   = e->totalDistractionsCount();
    r.distractionSeconds = e->totalDistractionSeconds();
    r.earlyFinish    = (outcome == "early_done");
    r.outcome        = outcome;
    return r;
}

void DashboardWidget::setupConnections()
{
    connect(m_engine, &FocusEngine::speedChanged,       m_speedGauge, &SpeedGaugeWidget::updateSpeed);
    connect(m_engine, &FocusEngine::speedChanged,       m_mapWidget,  &MapWidget::updateSpeed);
    connect(m_engine, &FocusEngine::rpmChanged,         m_rpmBar,     &RpmBarWidget::updateRpm);
    connect(m_engine, &FocusEngine::gearChanged,        m_gearWidget, &GearWidget::updateGear);
    connect(m_engine, &FocusEngine::progressChanged,    m_mapWidget,  &MapWidget::updateProgress);
    connect(m_engine, &FocusEngine::gearShiftTriggered, this,         &DashboardWidget::onGearShifted);
    connect(m_engine, &FocusEngine::distractionDetected,this,         &DashboardWidget::onDistraction);
    connect(m_engine, &FocusEngine::sessionCompleted,   this,         &DashboardWidget::onSessionCompleted);
    connect(m_engine, &FocusEngine::focusResumed, this, [this]() {
        m_statusLabel->setText("DRIVING  —  STAY FOCUSED");
        m_statusLabel->setStyleSheet("color: rgba(0,210,255,210); background: transparent;");
    });
    connect(m_engine, &FocusEngine::stateChanged, this, [this]{ onStateChanged(); });

    connect(m_startCancelBtn, &QPushButton::clicked, this, [this]() {
        if (m_engine->state() == FocusEngine::State::Running) {
            if (m_engine->focusTimeSeconds() >= 300) {
                // ≥5 min: "DONE" — save record then reset (no resume possible)
                SessionHistory::instance().append(buildRecord(m_engine, "early_done"));
                m_engine->reset();  // reset clears state, prevents resume
                m_statusLabel->setText("SESSION DONE  —  GREAT WORK!");
                m_statusLabel->setStyleSheet("color: rgba(0,220,130,230); background: transparent;");
                refreshStartButton();
            } else {
                // <5 min: "CANCEL" — discard
                m_engine->reset();
            }
        } else {
            m_engine->start();   // START
        }
    });
    connect(m_resetBtn,  &QPushButton::clicked, m_engine, &FocusEngine::reset);

    // Mute toggle
    connect(m_muteBtn, &QPushButton::clicked, this, [this]() {
        bool nowMuted = !m_engine->audioMuted();
        m_engine->setAudioMuted(nowMuted);
        m_muteBtn->setText("");
        styleButton(m_muteBtn, nowMuted ? QColor(100, 60, 60) : QColor(80, 130, 180));
        setButtonIcon(m_muteBtn, nowMuted ? QString(QChar(0xE74F)) : QString(QChar(0xE767)),
                      nowMuted ? QColor(100, 60, 60) : QColor(80, 130, 180));
    });



    // Allowed apps dialog
    connect(m_appsBtn, &QPushButton::clicked, this, [this]() {
        AllowedAppsDialog dlg(this);
        dlg.exec();
    });

    // History dialog
    connect(m_historyBtn, &QPushButton::clicked, this, [this]() {
        HistoryDialog dlg(this);
        dlg.exec();
    });
}

// ─────────────────────────────────────────────────────────────────────────────
void DashboardWidget::refreshStartButton()
{
    auto state = m_engine->state();
    bool running = (state == FocusEngine::State::Running);

    if (running) {
        if (m_engine->focusTimeSeconds() >= 300) {
            m_startCancelBtn->setText("DONE");
            styleButton(m_startCancelBtn, QColor(0, 185, 90));
            setButtonIcon(m_startCancelBtn, QString(QChar(0xE73E)), QColor(0, 185, 90));  // Checkmark
        } else {
            m_startCancelBtn->setText("CANCEL");
            styleButton(m_startCancelBtn, QColor(200, 60, 60));
            setButtonIcon(m_startCancelBtn, QString(QChar(0xE711)), QColor(200, 60, 60));  // X/Close
        }
    } else {
        m_startCancelBtn->setText("START");
        styleButton(m_startCancelBtn, QColor(0, 185, 90));
        setButtonIcon(m_startCancelBtn, QString(QChar(0xE768)), QColor(0, 185, 90));  // Play
    }
}

void DashboardWidget::onStateChanged()
{
    switch (m_engine->state()) {
    case FocusEngine::State::Running:
        m_statusLabel->setText("DRIVING  —  STAY FOCUSED");
        m_statusLabel->setStyleSheet("color: rgba(0,210,255,210); background: transparent;"); break;
    case FocusEngine::State::Idle:
        m_statusLabel->setText("READY");
        m_statusLabel->setStyleSheet("color: rgba(80,140,185,200); background: transparent;"); break;
    }
    refreshStartButton();
}

void DashboardWidget::onGearShifted(int gear)
{
    m_gearWidget->triggerShiftAnimation();
    m_speedGauge->triggerGlowEffect();
    if (m_flashAnim->state() == QAbstractAnimation::Running) m_flashAnim->stop();
    m_flashAnim->start();
    m_statusLabel->setText(QString("GEAR  %1  ENGAGED").arg(gear));
    m_statusLabel->setStyleSheet("color: rgba(0,240,255,230); background: transparent;");
}

void DashboardWidget::onDistraction(const QString &reason)
{
    if (m_warnAnim->state() == QAbstractAnimation::Running) m_warnAnim->stop();
    m_warnAnim->start();
    m_statusLabel->setText(QString("⚠  %1").arg(reason.toUpper()));
    m_statusLabel->setStyleSheet("color: rgba(255,100,60,230); background: transparent;");
}

void DashboardWidget::onSessionCompleted()
{
    SessionHistory::instance().append(buildRecord(m_engine, "auto_complete"));
    m_statusLabel->setText("SESSION COMPLETE  —  WELL DONE!");
    m_statusLabel->setStyleSheet("color: rgba(0,220,130,230); background: transparent;");
    refreshStartButton();
}

void DashboardWidget::updateHUD()
{
    int elapsed = m_engine->focusTimeSeconds();
    int goal    = m_engine->sessionGoal();
    int remain  = std::max(0, goal - elapsed);
    m_timerLabel->setTimes(elapsed, remain);
    // Refresh button label every tick — transitions CANCEL→DONE at 5min mark
    refreshStartButton();
    int d  = m_engine->totalDistractionsCount();
    int ds = m_engine->totalDistractionSeconds();
    int dm = ds / 60, dss = ds % 60;
    if (d > 0 || ds > 0) {
        QString timeStr = ds > 0 ? QString("  ·  lost %1m%2s")
            .arg(dm).arg(dss,2,10,QChar('0')) : "";
        m_statsLabel->setText(QString("Dist %1%2").arg(d).arg(timeStr));
    } else {
        m_statsLabel->setText("");
    }
}

void DashboardWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    QRectF r(0,0,width(),height());

    QLinearGradient bg(r.topLeft(), r.bottomRight());
    bg.setColorAt(0.0, QColor(5,10,18)); bg.setColorAt(0.5, QColor(8,15,27)); bg.setColorAt(1.0, QColor(4,9,17));
    p.fillRect(r, bg);

    p.setPen(QPen(QColor(0,60,110,18),1));
    for (int x=0;x<width();x+=44) p.drawLine(x,0,x,height());
    for (int y=0;y<height();y+=44) p.drawLine(0,y,width(),y);

    QRadialGradient vig(r.center(), std::max(width(),height())*0.72);
    vig.setColorAt(0.5,QColor(0,0,0,0)); vig.setColorAt(1.0,QColor(0,0,0,110));
    p.fillRect(r,vig);

    p.setPen(QPen(QColor(0,100,160,55),1.5)); p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(r.adjusted(1,1,-1,-1),4,4);

    if (m_flashAlpha>0.01) p.fillRect(r, QColor(0,195,255,static_cast<int>(m_flashAlpha*28)));
    if (m_warnAlpha>0.01)  p.fillRect(r, QColor(255,55,20,static_cast<int>(m_warnAlpha*38)));
}
