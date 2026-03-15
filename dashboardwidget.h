#pragma once

#include <QWidget>
#include <QPropertyAnimation>
#include <QLabel>
#include "etawidget.h"
#include "historydialog.h"
#include "sessionhistory.h"
#include "sessionrecord.h"
#include <QPushButton>

#include <QTimer>

class FocusEngine;
class SpeedGaugeWidget;
class GearWidget;
class MapWidget;
class RpmBarWidget;

class DashboardWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(double flashAlpha READ flashAlpha WRITE setFlashAlpha)
    Q_PROPERTY(double warnAlpha  READ warnAlpha  WRITE setWarnAlpha)

public:
    explicit DashboardWidget(QWidget *parent = nullptr);

    double flashAlpha() const { return m_flashAlpha; }
    void   setFlashAlpha(double v) { m_flashAlpha = v; update(); }

    double warnAlpha() const { return m_warnAlpha; }
    void   setWarnAlpha(double v) { m_warnAlpha = v; update(); }

protected:
    void paintEvent(QPaintEvent *) override;

private slots:
    void onGearShifted(int gear);
    void onDistraction(const QString &reason);
    void onSessionCompleted();
    void onStateChanged();
    void updateHUD();

private:
    void setupUI();
    void setupConnections();
    void styleButton(QPushButton *btn, const QColor &accent);
    void setButtonIcon(QPushButton *btn, const QString &glyph, const QColor &col);
    void refreshStartButton();

    FocusEngine      *m_engine{nullptr};
    SpeedGaugeWidget *m_speedGauge{nullptr};
    GearWidget       *m_gearWidget{nullptr};
    MapWidget        *m_mapWidget{nullptr};
    RpmBarWidget     *m_rpmBar{nullptr};

    QPushButton *m_startCancelBtn{nullptr};
    QPushButton *m_pauseBtn{nullptr};
    QPushButton *m_resetBtn{nullptr};
    QPushButton *m_muteBtn{nullptr};
    QPushButton *m_appsBtn{nullptr};
    QPushButton *m_historyBtn{nullptr};

    EtaWidget *m_timerLabel{nullptr};
    QLabel *m_statusLabel{nullptr};
    QLabel *m_statsLabel{nullptr};

    QTimer *m_hudTimer{nullptr};

    QPropertyAnimation *m_flashAnim{nullptr};
    QPropertyAnimation *m_warnAnim{nullptr};
    double m_flashAlpha{0.0};
    double m_warnAlpha{0.0};
};
