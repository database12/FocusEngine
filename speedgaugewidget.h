#pragma once

#include <QWidget>
#include <QPropertyAnimation>

class SpeedGaugeWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(double displaySpeed READ displaySpeed WRITE setDisplaySpeed)
    Q_PROPERTY(double glowPulse    READ glowPulse    WRITE setGlowPulse)

public:
    explicit SpeedGaugeWidget(QWidget *parent = nullptr);

    double displaySpeed() const { return m_displaySpeed; }
    void   setDisplaySpeed(double v) { m_displaySpeed = v; update(); }

    double glowPulse() const { return m_glowPulse; }
    void   setGlowPulse(double v) { m_glowPulse = v; update(); }

    void triggerGlowEffect();

public slots:
    void updateSpeed(double speed);

protected:
    void paintEvent(QPaintEvent *) override;
    QSize sizeHint()        const override { return {320, 320}; }
    QSize minimumSizeHint() const override { return {220, 220}; }

private:
    void drawGlassRing   (QPainter &p, QPointF c, double r);
    void drawArcTrack    (QPainter &p, QPointF c, double r);
    void drawTicks       (QPainter &p, QPointF c, double r);
    void drawNeedle      (QPainter &p, QPointF c, double r);
    void drawDigitalSpeed(QPainter &p, QPointF c, double r);

    // Map 0-180 km/h to angle in degrees (Qt paint space)
    // Gauge sweeps 240 degrees, start at 150° (bottom-left), end at 390°=30° (bottom-right)
    double speedToAngleDeg(double speed) const;

    double m_displaySpeed{0.0};
    double m_glowPulse{0.0};

    QPropertyAnimation *m_speedAnim{nullptr};
    QPropertyAnimation *m_glowAnim{nullptr};
};
