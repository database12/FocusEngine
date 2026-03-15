#include "speedgaugewidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QConicalGradient>
#include <QRadialGradient>
#include <QLinearGradient>
#include <cmath>

// Gauge arc: sweeps 240°, from 150° to 390° (= 30°) measured from Qt's 3-o'clock CCW convention
// We store angles as "degrees from 3-o'clock, CCW positive" (standard math)
// 150° from 3 o'clock CCW  = lower-left  (0 km/h)
// 150 - 240 = -90° = 270°  = top         (120 km/h midpoint)
// 150 - 240 = -90°, going to 150-240=-90 no...
// Simpler: use QPainter::drawArc convention (16ths of a degree, CCW from 3-o'clock)
// startAngle = 210 * 16 (lower-left), spanAngle = -240 * 16 (clockwise sweep)
static constexpr double GAUGE_START_DEG = 210.0; // Qt CCW from 3 o'clock
static constexpr double GAUGE_SPAN_DEG  = -240.0; // clockwise
static constexpr double MAX_SPEED       = 180.0;

SpeedGaugeWidget::SpeedGaugeWidget(QWidget *parent) : QWidget(parent)
{
    setMinimumSize(220, 220);

    m_speedAnim = new QPropertyAnimation(this, "displaySpeed", this);
    m_speedAnim->setDuration(700);
    m_speedAnim->setEasingCurve(QEasingCurve::OutCubic);

    m_glowAnim = new QPropertyAnimation(this, "glowPulse", this);
    m_glowAnim->setDuration(700);
    m_glowAnim->setEasingCurve(QEasingCurve::OutQuad);
}

void SpeedGaugeWidget::updateSpeed(double speed)
{
    if (m_speedAnim->state() == QAbstractAnimation::Running) m_speedAnim->stop();
    m_speedAnim->setStartValue(m_displaySpeed);
    m_speedAnim->setEndValue(speed);
    m_speedAnim->start();
}

void SpeedGaugeWidget::triggerGlowEffect()
{
    if (m_glowAnim->state() == QAbstractAnimation::Running) m_glowAnim->stop();
    m_glowAnim->setStartValue(1.0);
    m_glowAnim->setEndValue(0.0);
    m_glowAnim->start();
}

// Convert speed → angle in Qt drawArc degrees (CCW from 3 o'clock)
double SpeedGaugeWidget::speedToAngleDeg(double speed) const
{
    double t = std::clamp(speed, 0.0, MAX_SPEED) / MAX_SPEED;
    return GAUGE_START_DEG + t * GAUGE_SPAN_DEG;
}

void SpeedGaugeWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);

    double sz = std::min(width(), height()) - 16.0;
    QPointF c(width() / 2.0, height() / 2.0);
    double  r = sz / 2.0;

    drawGlassRing(p, c, r);
    drawArcTrack (p, c, r);
    drawTicks    (p, c, r);
    drawNeedle   (p, c, r);
    drawDigitalSpeed(p, c, r);
}

void SpeedGaugeWidget::drawGlassRing(QPainter &p, QPointF c, double r)
{
    // Outermost dark bezel
    QRadialGradient bezel(c, r + 8);
    bezel.setColorAt(0.82, QColor(18, 28, 42));
    bezel.setColorAt(0.90, QColor(8,  16, 28));
    bezel.setColorAt(1.00, QColor(0, 80, 150, 50));
    p.setBrush(bezel);
    p.setPen(Qt::NoPen);
    p.drawEllipse(c, r + 8, r + 8);

    // Main face
    QRadialGradient face(QPointF(c.x(), c.y() - r * 0.2), r * 1.1);
    face.setColorAt(0.00, QColor(22, 38, 58));
    face.setColorAt(0.65, QColor(11, 20, 34));
    face.setColorAt(1.00, QColor(5,  12, 22));
    p.setBrush(face);
    p.setPen(Qt::NoPen);
    p.drawEllipse(c, r, r);

    // Glow pulse ring on gear shift
    if (m_glowPulse > 0.01) {
        int a = static_cast<int>(m_glowPulse * 110);
        QPen glowPen(QColor(0, 200, 255, a), r * 0.04);
        p.setPen(glowPen);
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(c, r * 0.97, r * 0.97);
    }

    // Thin chrome rim
    QPen rimPen;
    rimPen.setWidthF(1.4);
    rimPen.setColor(QColor(60, 110, 160, 90));
    p.setPen(rimPen);
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(c, r, r);
}

void SpeedGaugeWidget::drawArcTrack(QPainter &p, QPointF c, double r)
{
    double trackR = r * 0.83;
    QRectF arcRect(c.x() - trackR, c.y() - trackR, trackR * 2, trackR * 2);
    double trackW = r * 0.055;

    // ── Background (inactive) track ─────────────────────────────
    QPen bgPen(QColor(30, 50, 72), trackW);
    bgPen.setCapStyle(Qt::FlatCap);
    p.setPen(bgPen);
    p.setBrush(Qt::NoBrush);
    p.drawArc(arcRect,
              static_cast<int>(GAUGE_START_DEG * 16),
              static_cast<int>(GAUGE_SPAN_DEG  * 16));

    // ── Active (filled) portion ──────────────────────────────────
    double t = m_displaySpeed / MAX_SPEED;
    double activeSweep = GAUGE_SPAN_DEG * t;
    if (std::abs(activeSweep) < 0.5) return;

    // Colour shifts: blue → cyan → orange → red
    QColor arcColor;
    if (t < 0.33)      arcColor = QColor(0,   160, 255);
    else if (t < 0.66) arcColor = QColor(0,   220, 200);
    else if (t < 0.88) arcColor = QColor(255, 140,   0);
    else               arcColor = QColor(255,  50,  50);

    // Soft glow under arc
    QPen glowPen(QColor(arcColor.red(), arcColor.green(), arcColor.blue(), 40),
                 trackW * 3.5);
    glowPen.setCapStyle(Qt::FlatCap);
    p.setPen(glowPen);
    p.drawArc(arcRect,
              static_cast<int>(GAUGE_START_DEG * 16),
              static_cast<int>(activeSweep * 16));

    // Bright arc on top
    QPen activePen(arcColor, trackW);
    activePen.setCapStyle(Qt::FlatCap);
    p.setPen(activePen);
    p.drawArc(arcRect,
              static_cast<int>(GAUGE_START_DEG * 16),
              static_cast<int>(activeSweep * 16));

    // Bright tip dot
    double tipAngleRad = qDegreesToRadians(speedToAngleDeg(m_displaySpeed));
    QPointF tip(c.x() + std::cos(tipAngleRad) * trackR,
                c.y() - std::sin(tipAngleRad) * trackR);
    QRadialGradient tipGlow(tip, trackW * 1.4);
    tipGlow.setColorAt(0, arcColor);
    tipGlow.setColorAt(1, QColor(arcColor.red(), arcColor.green(), arcColor.blue(), 0));
    p.setBrush(tipGlow);
    p.setPen(Qt::NoPen);
    p.drawEllipse(tip, trackW * 1.4, trackW * 1.4);
}

void SpeedGaugeWidget::drawTicks(QPainter &p, QPointF c, double r)
{
    // Major ticks every 20 km/h, minor every 10 km/h
    for (int spd = 0; spd <= 180; spd += 10) {
        bool major = (spd % 20 == 0);
        double t = static_cast<double>(spd) / MAX_SPEED;
        double angleDeg = GAUGE_START_DEG + t * GAUGE_SPAN_DEG;
        double rad = qDegreesToRadians(angleDeg);
        double cosA =  std::cos(rad);
        double sinA = -std::sin(rad);

        double outerR = r * 0.80;
        double innerR = major ? r * 0.70 : r * 0.75;

        QPointF outer(c.x() + cosA * outerR, c.y() + sinA * outerR);
        QPointF inner(c.x() + cosA * innerR, c.y() + sinA * innerR);

        QColor tickColor = major ? QColor(140, 190, 230, 200) : QColor(70, 110, 150, 130);
        p.setPen(QPen(tickColor, major ? 1.8 : 1.0));
        p.drawLine(outer, inner);

        // Major labels — rect must scale with r to avoid clipping
        if (major) {
            double labelR  = r * 0.59;
            double halfW   = r * 0.18;   // wide enough for "180"
            double halfH   = r * 0.12;
            QPointF labelCenter(c.x() + cosA * labelR, c.y() + sinA * labelR);
            QRectF  labelRect(labelCenter.x() - halfW, labelCenter.y() - halfH,
                             halfW * 2, halfH * 2);

            QFont f;
            f.setPixelSize(static_cast<int>(r * 0.105));
            f.setWeight(QFont::Medium);
            p.setFont(f);
            p.setPen(QColor(160, 200, 230, 200));
            p.drawText(labelRect, Qt::AlignCenter, QString::number(spd));
        }
    }
}

void SpeedGaugeWidget::drawNeedle(QPainter &p, QPointF c, double r)
{
    double angleDeg = speedToAngleDeg(m_displaySpeed);
    double rad      = qDegreesToRadians(angleDeg);
    double cosA     =  std::cos(rad);
    double sinA     = -std::sin(rad);

    double tipLen  = r * 0.72;
    double tailLen = r * 0.18;
    QPointF tip (c.x() + cosA * tipLen,  c.y() + sinA * tipLen);
    QPointF tail(c.x() - cosA * tailLen, c.y() - sinA * tailLen);

    // Wide glow
    QPen glow1(QColor(255, 120, 0, 25), r * 0.10);
    glow1.setCapStyle(Qt::RoundCap);
    p.setPen(glow1);
    p.drawLine(tail, tip);

    QPen glow2(QColor(255, 150, 30, 60), r * 0.05);
    glow2.setCapStyle(Qt::RoundCap);
    p.setPen(glow2);
    p.drawLine(tail, tip);

    // Needle body — thin white/orange line
    QPen needle(QColor(255, 230, 180), 2.2);
    needle.setCapStyle(Qt::RoundCap);
    p.setPen(needle);
    p.drawLine(tail, tip);

    // Center hub — small flat disc
    QRadialGradient hub(c, r * 0.06);
    hub.setColorAt(0.0, QColor(200, 210, 220));
    hub.setColorAt(0.5, QColor(60,  80, 110));
    hub.setColorAt(1.0, QColor(20,  35,  55));
    p.setBrush(hub);
    p.setPen(QPen(QColor(100, 140, 180, 120), 1.0));
    p.drawEllipse(c, r * 0.055, r * 0.055);
}

void SpeedGaugeWidget::drawDigitalSpeed(QPainter &p, QPointF c, double r)
{
    // Large digital readout in lower-center
    int spd = static_cast<int>(std::round(m_displaySpeed));

    // Value
    QFont valFont;
    valFont.setFamily("Courier New");
    valFont.setPixelSize(static_cast<int>(r * 0.34));
    valFont.setBold(true);
    p.setFont(valFont);

    QRectF valRect(c.x() - r * 0.45, c.y() + r * 0.16, r * 0.90, r * 0.40);

    // Faint shadow layers for glow
    for (int i = 2; i >= 1; --i) {
        p.setPen(QColor(0, 190, 255, 25 * i));
        p.drawText(valRect.adjusted(-i, -i, i, i), Qt::AlignCenter, QString::number(spd));
    }
    p.setPen(QColor(220, 240, 255));
    p.drawText(valRect, Qt::AlignCenter, QString::number(spd));

    // Unit label
    QFont unitFont;
    unitFont.setPixelSize(static_cast<int>(r * 0.11));
    unitFont.setLetterSpacing(QFont::AbsoluteSpacing, 1.5);
    p.setFont(unitFont);
    p.setPen(QColor(80, 130, 170, 180));
    p.drawText(QRectF(c.x() - r * 0.3, c.y() + r * 0.53, r * 0.6, r * 0.18),
               Qt::AlignCenter, "km/h");
}
