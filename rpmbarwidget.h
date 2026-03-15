#pragma once
#include <QWidget>
#include <QPropertyAnimation>
#include <QTimer>

class RpmBarWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(double displayRpm READ displayRpm WRITE setDisplayRpm)
    Q_PROPERTY(double glowPulse  READ glowPulse  WRITE setGlowPulse)

public:
    explicit RpmBarWidget(QWidget *parent = nullptr);

    static constexpr double RPM_MAX = 8000.0;

    double displayRpm() const { return m_displayRpm; }
    void   setDisplayRpm(double v) { m_displayRpm = v; update(); }

    double glowPulse() const { return m_glowPulse; }
    void   setGlowPulse(double v) { m_glowPulse = v; update(); }

public slots:
    void updateRpm(double rpm);
    void triggerGlowEffect();

protected:
    void paintEvent(QPaintEvent *) override;

private slots:
    void onFlash();

private:
    double rpmToAngleDeg(double rpm) const;
    void drawGlassRing   (QPainter &p, QPointF c, double r);
    void drawArcTrack    (QPainter &p, QPointF c, double r);
    void drawTicks       (QPainter &p, QPointF c, double r);
    void drawNeedle      (QPainter &p, QPointF c, double r);
    void drawDigitalRpm  (QPainter &p, QPointF c, double r);

    double m_displayRpm{0.0};
    double m_glowPulse{0.0};

    QPropertyAnimation *m_rpmAnim{nullptr};
    QPropertyAnimation *m_glowAnim{nullptr};

    QTimer *m_flashTimer{nullptr};
    bool    m_flashOn{false};

    // Same arc geometry as SpeedGaugeWidget
    static constexpr double GAUGE_START_DEG = 210.0;
    static constexpr double GAUGE_SPAN_DEG  = -240.0;
    static constexpr double REDLINE_RPM     = 7000.0;
};
