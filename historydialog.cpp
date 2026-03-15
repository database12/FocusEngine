#include "historydialog.h"
#include "sessionhistory.h"
#include <QPainter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QScrollArea>
#include <QMouseEvent>
#include <QDate>
#include <QMap>
#include <cmath>

// ─────────────────────────────────────────────────────────────────────────────
static QString sharedDark()
{
    return R"(
    * { font-family: "Courier New"; font-size: 13px; color: rgba(170,205,225,210); background: transparent; }
    QDialog, QWidget#root { background: #070f1c; }
    /* ── Calendar widget ── */
    QCalendarWidget {
        background: #070f1c;
    }
    QCalendarWidget QAbstractItemView {
        background: #070f1c;
        color: rgba(160,200,225,210);
        selection-background-color: rgba(0,140,200,90);
        selection-color: rgba(0,225,255,255);
        outline: none;
        font-size: 11px;
    }
    QCalendarWidget QAbstractItemView:disabled {
        color: rgba(55,85,105,130);
    }
    /* Header row: Sun Mon Tue ... */
    QCalendarWidget QHeaderView {
        background: #050c17;
        color: rgba(0,160,200,180);
        font-size: 10px;
    }
    QCalendarWidget QHeaderView::section {
        background: #050c17;
        color: rgba(0,170,210,170);
        border: none;
        padding: 3px 0px;
        font-size: 10px;
        letter-spacing: 1px;
    }
    /* Navigation bar */
    QCalendarWidget QWidget#qt_calendar_navigationbar {
        background: #050c17;
        border-bottom: 1px solid rgba(0,100,150,55);
        min-height: 32px;
    }
    QCalendarWidget QToolButton {
        color: rgba(0,195,235,200);
        background: transparent;
        border: none;
        font-family: "Courier New";
        font-size: 13px;
        padding: 2px 8px;
        letter-spacing: 1px;
    }
    QCalendarWidget QToolButton:hover {
        color: rgba(0,225,255,255);
        background: rgba(0,120,180,30);
        border-radius: 3px;
    }
    QCalendarWidget QToolButton::menu-indicator { image: none; width: 0px; }
    /* Month/year combo in navigation */
    QCalendarWidget QComboBox {
        color: rgba(0,195,235,200);
        background: #050c17;
        border: 1px solid rgba(0,100,150,70);
        border-radius: 3px;
        padding: 1px 20px 1px 6px;
        font-family: "Courier New";
        font-size: 13px;
        min-width: 90px;
    }
    QCalendarWidget QComboBox::drop-down {
        subcontrol-origin: padding;
        subcontrol-position: right center;
        width: 18px;
        border-left: 1px solid rgba(0,100,150,60);
    }
    QCalendarWidget QComboBox::down-arrow {
        width: 8px; height: 8px;
        border-left: 2px solid rgba(0,180,220,160);
        border-bottom: 2px solid rgba(0,180,220,160);
        transform: rotate(-45deg);
        margin-right: 3px;
    }
    QCalendarWidget QComboBox QAbstractItemView {
        background: #050c17;
        color: rgba(0,185,225,200);
        selection-background-color: rgba(0,130,185,70);
        border: 1px solid rgba(0,100,150,80);
    }
    QCalendarWidget QSpinBox {
        color: rgba(0,195,235,200);
        background: #050c17;
        border: 1px solid rgba(0,100,150,70);
        border-radius: 3px;
        padding: 1px 4px;
        font-family: "Courier New";
        font-size: 13px;
    }
    QCalendarWidget QSpinBox::up-button, QCalendarWidget QSpinBox::down-button {
        background: transparent; border: none;
    }
    QTableWidget {
        background: #050c17; border: 1px solid rgba(0,90,140,60);
        border-radius: 4px; outline: none; gridline-color: rgba(0,70,110,35);
    }
    QTableWidget::item { padding: 4px 8px; }
    QTableWidget::item:selected { background: rgba(0,130,190,50); color: rgba(0,220,255,240); }
    QTableWidget::item:hover    { background: rgba(0,90,140,25); }
    QHeaderView::section {
        background: #040b16; color: rgba(0,150,190,180);
        border: none; border-bottom: 1px solid rgba(0,90,140,60);
        padding: 4px 8px; font-family: "Courier New"; font-size: 10px;
        letter-spacing: 1px;
    }
    QScrollBar:vertical {
        background: #050c17; width: 8px;
        border: none; border-radius: 4px;
    }
    QScrollBar::handle:vertical {
        background: rgba(0,130,180,120); border-radius: 4px;
        min-height: 20px;
    }
    QScrollBar::handle:vertical:hover { background: rgba(0,170,220,180); }
    QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
        height: 0px; background: none; border: none;
    }
    QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
        background: #050c17;
    }
    QScrollBar:horizontal {
        background: #050c17; height: 8px;
        border: none; border-radius: 4px;
    }
    QScrollBar::handle:horizontal {
        background: rgba(0,130,180,120); border-radius: 4px;
        min-width: 20px;
    }
    QScrollBar::handle:horizontal:hover { background: rgba(0,170,220,180); }
    QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
        width: 0px; background: none; border: none;
    }
    QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal {
        background: #050c17;
    }
    QPushButton {
        background: rgba(0,90,140,25); color: rgba(0,185,225,200);
        border: 1px solid rgba(0,120,170,100); border-radius: 4px;
        padding: 4px 14px; font-size: 12px; letter-spacing: 1px;
    }
    QPushButton:hover    { background: rgba(0,130,185,60); border-color: rgba(0,190,230,160); }
    QPushButton:pressed  { background: rgba(0,150,200,100); }
    QPushButton:checked  { background: rgba(0,150,200,80); border-color: rgba(0,210,255,200);
                           color: rgba(0,225,255,255); }
    QPushButton#closeBtn { background: transparent; color: rgba(100,150,185,160);
                           border: none; font-size: 14px; min-width:32px; max-width:32px; }
    QPushButton#closeBtn:hover { color: rgba(220,70,70,240); background: rgba(200,30,30,40); }
    QLabel { background: transparent; }
    QComboBox {
        background: #050c17; border: 1px solid rgba(0,100,150,80);
        border-radius: 4px; padding: 3px 8px; color: rgba(0,185,225,200);
    }
    QComboBox::drop-down { border: none; }
    QComboBox QAbstractItemView { background: #050c17; color: rgba(0,185,225,200); }
    )";
}

// ─────────────────────────────────────────────────────────────────────────────
// FocusChartWidget
// ─────────────────────────────────────────────────────────────────────────────
FocusChartWidget::FocusChartWidget(QWidget *parent) : QWidget(parent)
{
    setMouseTracking(true);
}

void FocusChartWidget::setRange(const QDate &from, const QDate &to)
{
    m_from = from; m_to = to;
    m_data = SessionHistory::instance().dailyMinutes(from, to);
    m_maxVal = 1;
    for (auto v : m_data) m_maxVal = std::max(m_maxVal, v);
    update();
}

void FocusChartWidget::paintEvent(QPaintEvent *)
{
    if (!m_from.isValid()) return;
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const int n    = m_from.daysTo(m_to) + 1;
    const double w = width();
    const double h = height();
    const double padL = 36, padR = 8, padT = 12, padB = 22;
    const double plotW = w - padL - padR;
    const double plotH = h - padT - padB;
    const double barW  = std::max(2.0, plotW / n - 2.0);

    // Grid lines
    p.setPen(QPen(QColor(0, 80, 120, 50), 0.8));
    for (int i = 1; i <= 4; ++i) {
        double y = padT + plotH - plotH * i / 4.0;
        p.drawLine(QPointF(padL, y), QPointF(w - padR, y));
        QFont f("Courier New"); f.setPixelSize(10);
        p.setFont(f); p.setPen(QColor(0, 140, 180, 130));
        p.drawText(QRectF(0, y - 6, padL - 3, 12),
                   Qt::AlignRight | Qt::AlignVCenter,
                   QString::number(m_maxVal * i / 4));
        p.setPen(QPen(QColor(0, 80, 120, 50), 0.8));
    }

    // X axis
    p.setPen(QPen(QColor(0, 100, 150, 80), 0.8));
    p.drawLine(QPointF(padL, padT + plotH), QPointF(w - padR, padT + plotH));

    // Bars
    for (int i = 0; i < n; ++i) {
        QDate d = m_from.addDays(i);
        int   v = m_data.value(d, 0);
        double x = padL + (plotW / n) * i + (plotW / n - barW) / 2.0;
        double bh = (m_maxVal > 0) ? plotH * v / m_maxVal : 0;
        double by = padT + plotH - bh;

        bool isHL  = (d == m_highlight);
        bool today = (d == QDate::currentDate());

        // Bar fill
        QColor barCol = isHL  ? QColor(0, 220, 255, 200)
                      : today ? QColor(0, 180, 220, 150)
                      :         QColor(0, 130, 190, 100);
        if (v > 0) {
            QLinearGradient g(x, by, x, by + bh);
            g.setColorAt(0, barCol);
            g.setColorAt(1, QColor(barCol.red(), barCol.green(), barCol.blue(), 40));
            p.setBrush(g);
            p.setPen(isHL ? QPen(QColor(0, 230, 255, 200), 1) : Qt::NoPen);
            p.drawRoundedRect(QRectF(x, by, barW, bh), 2, 2);
        }

        // X label — day number or day-of-week abbrev
        QFont f("Courier New"); f.setPixelSize(10);
        p.setFont(f);
        p.setPen(isHL ? QColor(0,220,255,220)
                     : today ? QColor(0,180,220,180) : QColor(0,110,150,130));
        QString lbl = (n <= 7) ? d.toString("ddd") : QString::number(d.day());
        p.drawText(QRectF(x - 2, padT + plotH + 3, barW + 4, 14),
                   Qt::AlignCenter, lbl);

        // Value on top of bar
        if (v > 0 && bh > 14) {
            QFont vf("Courier New"); vf.setPixelSize(10); p.setFont(vf);
            p.setPen(QColor(0, 220, 255, 180));
            p.drawText(QRectF(x, by - 12, barW, 11), Qt::AlignCenter, QString::number(v));
        }
    }
}

void FocusChartWidget::mousePressEvent(QMouseEvent *e)
{
    if (!m_from.isValid()) return;
    const int n = m_from.daysTo(m_to) + 1;
    const double padL = 36, padR = 8;
    double plotW = width() - padL - padR;
    double x = e->pos().x() - padL;
    if (x < 0 || x > plotW) return;
    int i = static_cast<int>(x / (plotW / n));
    if (i >= 0 && i < n) emit dateClicked(m_from.addDays(i));
}

// ─────────────────────────────────────────────────────────────────────────────
// HistoryDialog
// ─────────────────────────────────────────────────────────────────────────────
HistoryDialog::HistoryDialog(QWidget *parent) : QDialog(parent)
{
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);
    setMinimumSize(820, 620);
    setStyleSheet(sharedDark());
    buildUi();
    onDateSelected(QDate::currentDate());
    refreshChart();
}

void HistoryDialog::buildUi()
{
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    // ── Title bar ────────────────────────────────────────────────
    auto *titleBar = new QWidget; titleBar->setFixedHeight(36);
    titleBar->setStyleSheet("background: qlineargradient(x1:0,y1:0,x2:1,y2:0,"
        "stop:0 #040c18, stop:0.5 #081422, stop:1 #040c18);"
        "border-bottom: 1px solid rgba(0,120,180,70);");
    auto *tb = new QHBoxLayout(titleBar);
    tb->setContentsMargins(14,0,6,0); tb->setSpacing(8);

    auto *dot = new QLabel("●");
    dot->setStyleSheet("color: rgba(0,200,255,160); font-size: 9px;");
    tb->addWidget(dot);

    auto *ttl = new QLabel("SESSION HISTORY");
    QFont tf("Courier New"); tf.setPixelSize(11); tf.setLetterSpacing(QFont::AbsoluteSpacing,3);
    ttl->setFont(tf);
    ttl->setStyleSheet("color: rgba(0,210,255,200); letter-spacing: 3px; font-size: 13px;");
    tb->addWidget(ttl);
    tb->addStretch();

    m_closeBtn = new QPushButton("✕");
    m_closeBtn->setObjectName("closeBtn");
    tb->addWidget(m_closeBtn);
    connect(m_closeBtn, &QPushButton::clicked, this, &QDialog::reject);
    outer->addWidget(titleBar);

    // ── Content ──────────────────────────────────────────────────
    auto *content = new QWidget;
    content->setObjectName("root");
    auto *mainLayout = new QVBoxLayout(content);
    mainLayout->setContentsMargins(14,10,14,12);
    mainLayout->setSpacing(10);
    outer->addWidget(content, 1);

    // ── Chart range selector ──────────────────────────────────────
    auto *rangeRow = new QHBoxLayout;
    auto *rangeLabel = new QLabel("SHOW:");
    QFont rl("Courier New"); rl.setPixelSize(9); rl.setLetterSpacing(QFont::AbsoluteSpacing,2);
    rangeLabel->setFont(rl);
    rangeLabel->setStyleSheet("color: rgba(0,160,200,190); font-size: 13px;");

    m_weekBtn  = new QPushButton("WEEK");
    m_monthBtn = new QPushButton("MONTH");
    m_weekBtn->setCheckable(true);
    m_monthBtn->setCheckable(true);
    m_weekBtn->setChecked(true);
    m_weekBtn->setFixedWidth(70);
    m_monthBtn->setFixedWidth(70);

    rangeRow->addWidget(rangeLabel);
    rangeRow->addWidget(m_weekBtn);
    rangeRow->addWidget(m_monthBtn);
    rangeRow->addStretch();

    connect(m_weekBtn,  &QPushButton::clicked, this, [this](){
        m_weekBtn->setChecked(true); m_monthBtn->setChecked(false); refreshChart(); });
    connect(m_monthBtn, &QPushButton::clicked, this, [this](){
        m_monthBtn->setChecked(true); m_weekBtn->setChecked(false); refreshChart(); });
    mainLayout->addLayout(rangeRow);

    // ── Chart ─────────────────────────────────────────────────────
    m_chart = new FocusChartWidget;
    m_chart->setFixedHeight(110);
    connect(m_chart, &FocusChartWidget::dateClicked, this, [this](const QDate &d){
        m_cal->setSelectedDate(d);
        onDateSelected(d);
    });
    mainLayout->addWidget(m_chart);

    // ── Lower section: calendar + session list ────────────────────
    auto *lower = new QHBoxLayout; lower->setSpacing(12);

    // Calendar
    m_cal = new QCalendarWidget;
    m_cal->setGridVisible(false);
    m_cal->setFixedWidth(260);
    m_cal->setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);
    m_cal->setNavigationBarVisible(true);

    // Force dark background on the day-of-week header row directly —
    // QSS on parent does not reach QHeaderView inside QCalendarWidget
    {
        QHeaderView *hv = m_cal->findChild<QHeaderView*>();
        if (hv) {
            hv->setStyleSheet(
                "QHeaderView { background: #070f1c; border: none; }"
                "QHeaderView::section {"
                "  background: #070f1c;"
                "  color: rgba(0,170,210,200);"
                "  border: none;"
                "  padding: 4px 0px;"
                "  font-family: 'Courier New';"
                "  font-size: 12px;"
                "}"
            );
        }
        // Also fix the table view background (the cell area)
        QAbstractItemView *tv = m_cal->findChild<QAbstractItemView*>();
        if (tv) {
            tv->setStyleSheet(
                "QAbstractItemView {"
                "  background: #070f1c;"
                "  alternate-background-color: #070f1c;"
                "  color: rgba(160,200,225,210);"
                "  selection-background-color: rgba(0,140,200,90);"
                "  selection-color: rgba(0,225,255,255);"
                "  border: none;"
                "  font-size: 13px;"
                "}"
            );
            tv->viewport()->setStyleSheet("background: #070f1c;");
        }
    }

    connect(m_cal, &QCalendarWidget::clicked, this, &HistoryDialog::onDateSelected);
    lower->addWidget(m_cal);

    // Right side: day label + session list
    auto *rightCol = new QVBoxLayout; rightCol->setSpacing(6);

    m_dayLabel = new QLabel("—");
    QFont dlf("Courier New"); dlf.setPixelSize(15); dlf.setLetterSpacing(QFont::AbsoluteSpacing,2);
    dlf.setBold(true);
    m_dayLabel->setFont(dlf);
    m_dayLabel->setStyleSheet("color: rgba(0,210,255,200);");
    rightCol->addWidget(m_dayLabel);

    m_summaryLabel = new QLabel("");
    QFont sf("Courier New"); sf.setPixelSize(13); sf.setLetterSpacing(QFont::AbsoluteSpacing,1);
    m_summaryLabel->setFont(sf);
    m_summaryLabel->setStyleSheet("color: rgba(0,150,190,160);");
    rightCol->addWidget(m_summaryLabel);

    // Columns: # | ✓ | Time | Elapsed | Goal | Speed | RPM | Gear | Dist | Task
    m_list = new QTableWidget(0, 9);
    m_list->setToolTip("Double-click Task cell to edit");
    m_list->setHorizontalHeaderLabels({
        "#", "", "TIME", "ELAPSED", "GEAR", "MAX SPD", "MAX RPM", "DISTRACTED", "TASK"
    });
    m_list->verticalHeader()->setVisible(false);
    m_list->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    m_list->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_list->setShowGrid(true);
    m_list->setAlternatingRowColors(false);
    m_list->horizontalHeader()->setStretchLastSection(true);
    m_list->horizontalHeader()->setHighlightSections(false);
    m_list->verticalHeader()->setDefaultSectionSize(28);

    // Column widths
    m_list->setColumnWidth(0, 30);   // #
    m_list->setColumnWidth(1, 24);   // icon
    m_list->setColumnWidth(2, 120);  // time
    m_list->setColumnWidth(3, 90);   // elapsed
    m_list->setColumnWidth(4, 45);   // gear
    m_list->setColumnWidth(5, 80);   // speed
    m_list->setColumnWidth(6, 80);   // rpm
    m_list->setColumnWidth(7, 90);   // distracted time
    // col 8 (task) stretches

    connect(m_list, &QTableWidget::cellDoubleClicked, this, [this](int row, int col) {
        if (col != 8) return;  // only task column editable
        if (row < 0 || row >= m_currentRecords.size()) return;
        const SessionRecord &r = m_currentRecords[row];

        auto *editor = new QWidget(this, Qt::Popup);
        editor->setStyleSheet(
            "background: #070f1c; border: 1px solid rgba(0,160,210,150); border-radius: 4px;");
        auto *lay = new QHBoxLayout(editor);
        lay->setContentsMargins(8,6,8,6); lay->setSpacing(6);

        auto *le = new QLineEdit(r.taskName);
        le->setPlaceholderText("Enter task name...");
        le->setStyleSheet(
            "QLineEdit { background: #050c17; color: rgba(0,220,255,220);"
            "  border: 1px solid rgba(0,140,200,120); border-radius: 3px;"
            "  padding: 4px 8px; font-family: 'Courier New'; font-size: 12px; }");
        le->setMinimumWidth(320);
        le->selectAll();

        auto *ok = new QPushButton("OK");
        ok->setStyleSheet(
            "QPushButton { background: rgba(0,140,200,40); color: rgba(0,210,255,220);"
            "  border: 1px solid rgba(0,140,200,120); border-radius: 3px; padding: 4px 12px;"
            "  font-family: 'Courier New'; font-size: 12px; }"
            "QPushButton:hover { background: rgba(0,160,220,80); }");
        lay->addWidget(le); lay->addWidget(ok);

        QRect cellRect = m_list->visualRect(m_list->model()->index(row, col));
        QPoint gpos = m_list->viewport()->mapToGlobal(cellRect.bottomLeft());
        editor->move(gpos);
        editor->show();
        le->setFocus();

        QDateTime startTime = r.startTime;
        auto save = [this, le, editor, startTime, row]() {
            QString name = le->text().trimmed();
            SessionHistory::instance().updateTaskName(startTime, name);
            if (row < m_currentRecords.size()) {
                m_currentRecords[row].taskName = name;
                auto *cell = m_list->item(row, 8);
                if (cell) cell->setText(name);
            }
            editor->close(); editor->deleteLater();
        };
        connect(ok, &QPushButton::clicked, this, save);
        connect(le, &QLineEdit::returnPressed, this, save);
    });
    rightCol->addWidget(m_list, 1);
    lower->addLayout(rightCol, 1);

    mainLayout->addLayout(lower, 1);
}

// ─────────────────────────────────────────────────────────────────────────────
void HistoryDialog::refreshChart()
{
    QDate today = QDate::currentDate();
    if (m_weekBtn->isChecked()) {
        QDate from = today.addDays(-(today.dayOfWeek()-1));   // Monday
        m_chart->setRange(from, from.addDays(6));
    } else {
        m_chart->setRange(QDate(today.year(), today.month(), 1),
                          QDate(today.year(), today.month(), today.daysInMonth()));
    }
    m_chart->setHighlightDate(m_cal ? m_cal->selectedDate() : today);
}

void HistoryDialog::onDateSelected(const QDate &d)
{
    m_chart->setHighlightDate(d);
    m_dayLabel->setText(d.toString("dddd, dd MMM yyyy").toUpper());

    auto records = SessionHistory::instance().forDate(d);
    m_currentRecords = records;
    m_list->setRowCount(0);

    if (records.isEmpty()) {
        m_summaryLabel->setText("No sessions recorded for this day.");
        return;
    }

    int totalMin = 0;
    for (const auto &r : records) totalMin += r.elapsedSeconds / 60;
    m_summaryLabel->setText(QString("%1 session%2  ·  %3 min total focused")
        .arg(records.size()).arg(records.size()>1?"s":"").arg(totalMin));

    QFont cellFont("Courier New"); cellFont.setPixelSize(11);

    for (int i = 0; i < records.size(); ++i) {
        const auto &r = records[i];
        m_list->insertRow(i);

        QColor rowCol = (r.outcome == "completed" || r.outcome == "auto_complete")
                        ? QColor(0, 210, 130, 220)
                        : (r.outcome == "early_done") ? QColor(0, 195, 255, 200)
                        : QColor(140, 140, 140, 160);

        int em = r.elapsedSeconds / 60, es = r.elapsedSeconds % 60;
        int dm2 = r.distractionSeconds / 60, ds2 = r.distractionSeconds % 60;
        QString outcomeIcon = (r.outcome == "cancelled") ? "✕" : "✓";

        QStringList cols = {
            QString::number(i+1),
            outcomeIcon,
            r.startTime.toString("HH:mm") + " → " + r.endTime.toString("HH:mm"),
            QString("%1m%2s").arg(em,2,10,QChar('0')).arg(es,2,10,QChar('0')),
            QString("G%1").arg(r.peakGear),
            QString("%1 km/h").arg(static_cast<int>(r.maxSpeed)),
            QString("%1 RPM").arg(static_cast<int>(r.maxRpm)),
            r.distractions > 0
                ? QString("%1x %2m%3s").arg(r.distractions).arg(dm2).arg(ds2,2,10,QChar('0'))
                : QString("—"),
            r.taskName
        };

        for (int j = 0; j < cols.size(); ++j) {
            auto *cell = new QTableWidgetItem(cols[j]);
            cell->setFont(cellFont);
            cell->setForeground(j == 1 ? rowCol : (j == 8 ? QColor(0,180,220,200) : rowCol.lighter(110)));
            cell->setTextAlignment(j <= 1 ? Qt::AlignCenter : Qt::AlignVCenter | Qt::AlignLeft);
            cell->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            m_list->setItem(i, j, cell);
        }
    }
}


// ─── Drag-to-move ─────────────────────────────────────────────────────────
void HistoryDialog::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton && e->pos().y() < 36) {
        m_dragging = true;
        m_dragOffset = e->globalPosition().toPoint() - frameGeometry().topLeft();
    } else QDialog::mousePressEvent(e);
}
void HistoryDialog::mouseMoveEvent(QMouseEvent *e)
{
    if (m_dragging) move(e->globalPosition().toPoint() - m_dragOffset);
    else QDialog::mouseMoveEvent(e);
}
void HistoryDialog::mouseReleaseEvent(QMouseEvent *e)
{
    m_dragging = false;
    QDialog::mouseReleaseEvent(e);
}
