#pragma once
#include <QDialog>
#include <QCalendarWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QLabel>
#include <QMouseEvent>
#include <QLineEdit>
#include <QDate>
#include "sessionrecord.h"

// ── Inline bar chart widget ───────────────────────────────────────────────────
class FocusChartWidget : public QWidget
{
    Q_OBJECT
public:
    explicit FocusChartWidget(QWidget *parent = nullptr);
    void setRange(const QDate &from, const QDate &to);
    void setHighlightDate(const QDate &d) { m_highlight = d; update(); }
    QSize sizeHint() const override { return {0, 120}; }

signals:
    void dateClicked(const QDate &d);

protected:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *e) override;

private:
    QDate m_from, m_to, m_highlight;
    QMap<QDate,int> m_data;
    int m_maxVal{1};
};

// ── Main history dialog ───────────────────────────────────────────────────────
class HistoryDialog : public QDialog
{
    Q_OBJECT
public:
    explicit HistoryDialog(QWidget *parent = nullptr);

protected:
    void mousePressEvent  (QMouseEvent *e) override;
    void mouseMoveEvent   (QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;

private slots:
    void onDateSelected(const QDate &d);

private:
    void buildUi();
    void refreshList(const QDate &d);
    void refreshChart();

    QCalendarWidget  *m_cal      {nullptr};
    QTableWidget     *m_list     {nullptr};
    QLabel           *m_dayLabel {nullptr};
    QLabel           *m_summaryLabel{nullptr};
    FocusChartWidget *m_chart    {nullptr};
    QPushButton      *m_closeBtn {nullptr};
    QPushButton      *m_weekBtn  {nullptr};
    QPushButton      *m_monthBtn {nullptr};

    QVector<SessionRecord> m_currentRecords;
    bool   m_dragging   {false};
    QPoint m_dragOffset;
};
