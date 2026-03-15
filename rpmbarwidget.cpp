#include "rpmbarwidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <QLinearGradient>
#include <cmath>

RpmBarWidget::RpmBarWidget(QWidget *parent) : QWidget(parent)
{
    setMinimumSize(180, 180);

    m_rpmAnim = new QPropertyAnimation(this, "displayRpm", this);
    m_rpmAnim->setDuration(500);
    m_rpmAnim->setEasingCurve(QEasingCurve::OutCubic);

    m_glowAnim = new QPropertyAnimation(this, "glowPulse", this);
    m_glowAnim->setDuration(700);
    m_glowAnim->setEasingCurve(QEasingCurve::OutQuad);

    m_flashTimer = new QTimer(this);
    m_flashTimer->setInterval(130);
    connect(m_flashTimer, &QTimer::timeout, this, &RpmBarWidget::onFlash);
}

void RpmBarWidget::updateRpm(double rpm)
{
    m_rpmAnim->stop();
    m_rpmAnim->setStartValue(m_displayRpm);
    m_rpmAnim->setEndValue(rpm);
    m_rpmAnim->start();

    bool redline = rpm >= REDLINE_RPM;
    if (redline  && !m_flashTimer->isActive()) m_flashTimer->start();
    if (!redline &&  m_flashTimer->isActive()) { m_flashTimer->stop(); m_flashOn = false; }
}

void RpmBarWidget::triggerGlowEffect()
{
    m_glowAnim->stop();
    m_glowAnim->setStartValue(1.0);
    m_glowAnim->setEndValue(0.0);
    m_glowAnim->start();
}

void RpmBarWidget::onFlash() { m_flashOn = !m_flashOn; update(); }

double RpmBarWidget::rpmToAngleDeg(double rpm) const
{
    double t = std::clamp(rpm, 0.0, RPM_MAX) / RPM_MAX;
    return GAUGE_START_DEG + t * GAUGE_SPAN_DEG;
}

// ─────────────────────────────────────────────────────────────────────────────
void RpmBarWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);

    double sz = std::min(width(), height()) - 16.0;
    QPointF c(width() / 2.0, height() / 2.0);
    double  r = sz / 2.0;

    drawGlassRing (p, c, r);
    drawArcTrack  (p, c, r);
    drawTicks     (p, c, r);
    drawNeedle    (p, c, r);
    drawDigitalRpm(p, c, r);
}

// ─── Glass ring — identical to SpeedGaugeWidget ──────────────────────────────
void RpmBarWidget::drawGlassRing(QPainter &p, QPointF c, double r)
{
    QRadialGradient bezel(c, r + 8);
    bezel.setColorAt(0.82, QColor(18, 28, 42));
    bezel.setColorAt(0.90, QColor(8,  16, 28));
    bezel.setColorAt(1.00, QColor(0, 80, 150, 50));
    p.setBrush(bezel); p.setPen(Qt::NoPen);
    p.drawEllipse(c, r + 8, r + 8);

    QRadialGradient face(QPointF(c.x(), c.y() - r * 0.2), r * 1.1);
    face.setColorAt(0.00, QColor(22, 38, 58));
    face.setColorAt(0.65, QColor(11, 20, 34));
    face.setColorAt(1.00, QColor(5,  12, 22));
    p.setBrush(face); p.setPen(Qt::NoPen);
    p.drawEllipse(c, r, r);

    if (m_glowPulse > 0.01) {
        int a = static_cast<int>(m_glowPulse * 110);
        p.setPen(QPen(QColor(0, 200, 255, a), r * 0.04));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(c, r * 0.97, r * 0.97);
    }

    // Redline flash outer ring
    if (m_flashOn) {
        p.setPen(QPen(QColor(220, 30, 30, 160), r * 0.035));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(c, r * 0.97, r * 0.97);
    }

    p.setPen(QPen(QColor(60, 110, 160, 90), 1.4));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(c, r, r);
}

// ─── Arc track ───────────────────────────────────────────────────────────────
void RpmBarWidget::drawArcTrack(QPainter &p, QPointF c, double r)
{
    double trackR = r * 0.83;
    QRectF arcRect(c.x() - trackR, c.y() - trackR, trackR * 2, trackR * 2);
    double trackW = r * 0.055;

    // Background track
    QPen bgPen(QColor(30, 50, 72), trackW);
    bgPen.setCapStyle(Qt::FlatCap);
    p.setPen(bgPen); p.setBrush(Qt::NoBrush);
    p.drawArc(arcRect,
              static_cast<int>(GAUGE_START_DEG * 16),
              static_cast<int>(GAUGE_SPAN_DEG  * 16));

    double t = m_displayRpm / RPM_MAX;
    double activeSweep = GAUGE_SPAN_DEG * t;
    if (std::abs(activeSweep) < 0.5) return;

    // Colour zones — green → amber → red (matching reference image)
    // Draw in 3 passes for the 3 colour zones
    struct Zone { double fromRpm, toRpm; QColor col; };
    Zone zones[] = {
        { 0,            5000, QColor( 30, 210,  90) },   // green
        { 5000, REDLINE_RPM,  QColor(220, 140,   0) },   // amber
        { REDLINE_RPM, RPM_MAX, m_flashOn ? QColor(255,80,80) : QColor(220, 35, 35) } // red
    };

    for (auto &z : zones) {
        double zFrom = std::clamp(z.fromRpm, 0.0, m_displayRpm);
        double zTo   = std::clamp(z.toRpm,   0.0, m_displayRpm);
        if (zTo <= zFrom) continue;

        double tFrom = zFrom / RPM_MAX;
        double tTo   = zTo   / RPM_MAX;
        double startDeg = GAUGE_START_DEG + tFrom * GAUGE_SPAN_DEG;
        double spanDeg  = (tTo - tFrom) * GAUGE_SPAN_DEG;

        // Glow
        QPen glowPen(QColor(z.col.red(), z.col.green(), z.col.blue(), 38), trackW * 3.5);
        glowPen.setCapStyle(Qt::FlatCap);
        p.setPen(glowPen);
        p.drawArc(arcRect,
                  static_cast<int>(startDeg * 16),
                  static_cast<int>(spanDeg  * 16));

        // Bright arc
        QPen arcPen(z.col, trackW);
        arcPen.setCapStyle(Qt::FlatCap);
        p.setPen(arcPen);
        p.drawArc(arcRect,
                  static_cast<int>(startDeg * 16),
                  static_cast<int>(spanDeg  * 16));
    }

    // Bright tip dot
    double tipAngleRad = qDegreesToRadians(rpmToAngleDeg(m_displayRpm));
    QPointF tip(c.x() + std::cos(tipAngleRad) * trackR,
                c.y() - std::sin(tipAngleRad) * trackR);
    QColor tipCol = (m_displayRpm >= REDLINE_RPM)
                        ? QColor(255, 60, 60)
                        : (m_displayRpm >= 5000 ? QColor(220,140,0) : QColor(30,210,90));
    QRadialGradient tipGlow(tip, trackW * 1.4);
    tipGlow.setColorAt(0, tipCol);
    tipGlow.setColorAt(1, QColor(tipCol.red(), tipCol.green(), tipCol.blue(), 0));
    p.setBrush(tipGlow); p.setPen(Qt::NoPen);
    p.drawEllipse(tip, trackW * 1.4, trackW * 1.4);

    // Redline marker line at 7000 RPM
    {
        double redlineAngleRad = qDegreesToRadians(rpmToAngleDeg(REDLINE_RPM));
        QPointF rl_inner(c.x() + std::cos(redlineAngleRad) * trackR * 0.88,
                         c.y() - std::sin(redlineAngleRad) * trackR * 0.88);
        QPointF rl_outer(c.x() + std::cos(redlineAngleRad) * trackR * 1.06,
                         c.y() - std::sin(redlineAngleRad) * trackR * 1.06);
        p.setPen(QPen(QColor(200, 40, 40, 180), 1.8));
        p.drawLine(rl_inner, rl_outer);
    }
}

// ─── Ticks — same style as speed gauge but for RPM (0–8 × 1000) ─────────────
void RpmBarWidget::drawTicks(QPainter &p, QPointF c, double r)
{
    // Major ticks at every 1000 RPM, minor at 500
    for (int rpm = 0; rpm <= 8000; rpm += 500) {
        bool major = (rpm % 1000 == 0);
        double t = static_cast<double>(rpm) / RPM_MAX;
        double angleDeg = GAUGE_START_DEG + t * GAUGE_SPAN_DEG;
        double rad  = qDegreesToRadians(angleDeg);
        double cosA =  std::cos(rad);
        double sinA = -std::sin(rad);

        double outerR = r * 0.80;
        double innerR = major ? r * 0.70 : r * 0.75;

        QColor tickCol = major ? QColor(140, 190, 230, 200) : QColor(70, 110, 150, 130);
        // Red ticks in redline zone
        if (rpm >= static_cast<int>(REDLINE_RPM))
            tickCol = QColor(200, 60, 60, 180);

        p.setPen(QPen(tickCol, major ? 1.8 : 1.0));
        p.drawLine(QPointF(c.x() + cosA * outerR, c.y() + sinA * outerR),
                   QPointF(c.x() + cosA * innerR, c.y() + sinA * innerR));

        if (major) {
            double labelR = r * 0.59;
            double halfW  = r * 0.14;
            double halfH  = r * 0.11;
            QPointF lc(c.x() + cosA * labelR, c.y() + sinA * labelR);
            QRectF  lr(lc.x() - halfW, lc.y() - halfH, halfW * 2, halfH * 2);
            QFont f; f.setPixelSize(static_cast<int>(r * 0.095));
            f.setWeight(QFont::Medium);
            p.setFont(f);
            // Label as "1", "2", ... "8"
            QColor lblCol = (rpm >= static_cast<int>(REDLINE_RPM))
                                ? QColor(200, 80, 80, 200) : QColor(140, 190, 230, 190);
            p.setPen(lblCol);
            p.drawText(lr, Qt::AlignCenter, QString::number(rpm / 1000));
        }
    }

    // "×1000" label at bottom
    QFont sf; sf.setPixelSize(static_cast<int>(r * 0.08));
    sf.setLetterSpacing(QFont::AbsoluteSpacing, 0.5);
    p.setFont(sf);
    p.setPen(QColor(70, 110, 150, 150));
    p.drawText(QRectF(c.x() - r * 0.5, c.y() + r * 0.70, r, r * 0.18),
               Qt::AlignCenter, "×1000 RPM");
}

// ─── Needle — identical to speed gauge ───────────────────────────────────────
void RpmBarWidget::drawNeedle(QPainter &p, QPointF c, double r)
{
    double angleDeg = rpmToAngleDeg(m_displayRpm);
    double rad      = qDegreesToRadians(angleDeg);
    double cosA     =  std::cos(rad);
    double sinA     = -std::sin(rad);

    QPointF tip (c.x() + cosA * r * 0.72, c.y() + sinA * r * 0.72);
    QPointF tail(c.x() - cosA * r * 0.18, c.y() - sinA * r * 0.18);

    // Needle colour: white/green normally, red in redline
    QColor needleCol = (m_displayRpm >= REDLINE_RPM)
                           ? (m_flashOn ? QColor(255,100,100) : QColor(220, 180, 180))
                           : QColor(255, 230, 180);

    QPen glow1(QColor(needleCol.red(), needleCol.green(), needleCol.blue(), 25), r * 0.10);
    glow1.setCapStyle(Qt::RoundCap); p.setPen(glow1); p.drawLine(tail, tip);

    QPen glow2(QColor(needleCol.red(), needleCol.green(), needleCol.blue(), 60), r * 0.05);
    glow2.setCapStyle(Qt::RoundCap); p.setPen(glow2); p.drawLine(tail, tip);

    QPen needle(needleCol, 2.2);
    needle.setCapStyle(Qt::RoundCap); p.setPen(needle); p.drawLine(tail, tip);

    // Hub
    QRadialGradient hub(c, r * 0.06);
    hub.setColorAt(0.0, QColor(200, 210, 220));
    hub.setColorAt(0.5, QColor(60,  80, 110));
    hub.setColorAt(1.0, QColor(20,  35,  55));
    p.setBrush(hub);
    p.setPen(QPen(QColor(100, 140, 180, 120), 1.0));
    p.drawEllipse(c, r * 0.055, r * 0.055);
}

// ─── Digital RPM readout — same position/style as speed gauge ────────────────
void RpmBarWidget::drawDigitalRpm(QPainter &p, QPointF c, double r)
{
    int rpm = static_cast<int>(std::round(m_displayRpm / 100.0)) * 100;

    QColor valCol = (m_displayRpm >= REDLINE_RPM)
                        ? (m_flashOn ? QColor(255, 100, 100) : QColor(230, 160, 160))
                        : QColor(220, 240, 255);

    QFont valFont;
    valFont.setFamily("Courier New");
    valFont.setPixelSize(static_cast<int>(r * 0.28));
    valFont.setBold(true);
    p.setFont(valFont);

    QRectF valRect(c.x() - r * 0.50, c.y() + r * 0.18, r * 1.0, r * 0.36);

    // Glow layers
    for (int i = 2; i >= 1; --i) {
        p.setPen(QColor(valCol.red(), valCol.green(), valCol.blue(), 22 * i));
        p.drawText(valRect.adjusted(-i,-i,i,i), Qt::AlignCenter, QString::number(rpm));
    }
    p.setPen(valCol);
    p.drawText(valRect, Qt::AlignCenter, QString::number(rpm));

    // Unit
    QFont unitFont;
    unitFont.setPixelSize(static_cast<int>(r * 0.10));
    unitFont.setLetterSpacing(QFont::AbsoluteSpacing, 1.5);
    p.setFont(unitFont);
    p.setPen(QColor(80, 130, 170, 180));
    p.drawText(QRectF(c.x() - r * 0.3, c.y() + r * 0.52, r * 0.6, r * 0.17),
               Qt::AlignCenter, "rpm");
}
