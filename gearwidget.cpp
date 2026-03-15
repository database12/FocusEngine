#include "gearwidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QRadialGradient>
#include <QLinearGradient>
#include <cmath>

GearWidget::GearWidget(QWidget *parent) : QWidget(parent)
{
    setMinimumSize(180, 180);

    m_flashAnim = new QPropertyAnimation(this, "flashAlpha", this);
    m_flashAnim->setDuration(500);
    m_flashAnim->setStartValue(1.0);
    m_flashAnim->setEndValue(0.0);
    m_flashAnim->setEasingCurve(QEasingCurve::OutCubic);

    m_scaleAnimObj = new QPropertyAnimation(this, "scaleAnim", this);
    m_scaleAnimObj->setDuration(380);
    m_scaleAnimObj->setStartValue(1.55);
    m_scaleAnimObj->setEndValue(1.0);
    m_scaleAnimObj->setEasingCurve(QEasingCurve::OutBack);

    m_group = new QParallelAnimationGroup(this);
    m_group->addAnimation(m_flashAnim);
    m_group->addAnimation(m_scaleAnimObj);
}

void GearWidget::updateGear(int gear)
{
    m_gear = gear;
    update();
}

void GearWidget::triggerShiftAnimation()
{
    if (m_group->state() == QAbstractAnimation::Running) m_group->stop();
    m_flashAnim->setStartValue(1.0);
    m_flashAnim->setEndValue(0.0);
    m_scaleAnimObj->setStartValue(1.55);
    m_scaleAnimObj->setEndValue(1.0);
    m_group->start();
}

void GearWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);

    double sz = std::min(width(), height()) - 16.0;
    QPointF c(width() / 2.0, height() / 2.0);
    double  r = sz / 2.0;

    drawBackground(p, c, r);
    drawTimerArc  (p, c, r);
    drawGearDots  (p, c, r);
    drawGearNumber(p, c, r);
}

void GearWidget::drawBackground(QPainter &p, QPointF c, double r)
{
    // Bezel
    QRadialGradient bezel(c, r + 8);
    bezel.setColorAt(0.82, QColor(18, 28, 42));
    bezel.setColorAt(0.90, QColor(8,  16, 28));
    bezel.setColorAt(1.00, QColor(0,  80, 150, 50));
    p.setBrush(bezel);
    p.setPen(Qt::NoPen);
    p.drawEllipse(c, r + 8, r + 8);

    // Face
    QRadialGradient face(QPointF(c.x(), c.y() - r * 0.2), r * 1.1);
    face.setColorAt(0.00, QColor(22, 38, 58));
    face.setColorAt(0.65, QColor(11, 20, 34));
    face.setColorAt(1.00, QColor(5,  12, 22));
    p.setBrush(face);
    p.setPen(Qt::NoPen);
    p.drawEllipse(c, r, r);

    // Flash overlay on gear shift
    if (m_flashAlpha > 0.01) {
        QRadialGradient flash(c, r * 0.7);
        flash.setColorAt(0, QColor(0, 210, 255, static_cast<int>(m_flashAlpha * 130)));
        flash.setColorAt(1, QColor(0, 140, 220, 0));
        p.setBrush(flash);
        p.setPen(Qt::NoPen);
        p.drawEllipse(c, r * 0.7, r * 0.7);
    }

    // Chrome rim
    p.setPen(QPen(QColor(60, 110, 160, 90), 1.4));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(c, r, r);
}

void GearWidget::drawTimerArc(QPainter &p, QPointF c, double r)
{
    // Shows progress within current pomodoro (inner ring)
    double pomodoroSeconds = 1500.0;
    // We don't have seconds here, so just show gear progress visually:
    // filled dots already communicate this — skip timer arc to keep it clean
    Q_UNUSED(pomodoroSeconds)

    // Just draw a subtle separator ring
    double sepR = r * 0.76;
    QPen sep(QColor(35, 55, 80, 120), 1.0);
    p.setPen(sep);
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(c, sepR, sepR);
}

void GearWidget::drawGearDots(QPainter &p, QPointF c, double r)
{
    // 6 dots evenly spaced on outer ring, lit for gears completed
    for (int i = 1; i <= 6; i++) {
        // Start at top, go clockwise
        double angleDeg = -90.0 + (i - 1) * 60.0;
        double angleRad = qDegreesToRadians(angleDeg);
        double dotR     = r * 0.80;
        QPointF dotC(c.x() + std::cos(angleRad) * dotR,
                     c.y() + std::sin(angleRad) * dotR);

        bool active = (i <= m_gear);
        bool current = (i == m_gear);

        if (active) {
            // Glow halo
            QRadialGradient halo(dotC, r * 0.09);
            halo.setColorAt(0, QColor(0, 200, 255, current ? 90 : 50));
            halo.setColorAt(1, QColor(0, 140, 220, 0));
            p.setBrush(halo);
            p.setPen(Qt::NoPen);
            p.drawEllipse(dotC, r * 0.09, r * 0.09);

            // Filled dot
            p.setBrush(current ? QColor(0, 230, 255) : QColor(0, 150, 200));
            p.setPen(QPen(QColor(80, 200, 240, 160), 1.0));
            p.drawEllipse(dotC, r * 0.050, r * 0.050);
        } else {
            p.setBrush(QColor(22, 38, 56));
            p.setPen(QPen(QColor(45, 70, 100, 140), 1.0));
            p.drawEllipse(dotC, r * 0.040, r * 0.040);
        }
    }
}

void GearWidget::drawGearNumber(QPainter &p, QPointF c, double r)
{
    QString text = QString::number(m_gear);

    p.save();
    p.translate(c);
    p.scale(m_scaleAnim, m_scaleAnim);
    p.translate(-c);

    // Glow layers
    QFont f;
    f.setFamily("Courier New");
    f.setPixelSize(static_cast<int>(r * 0.62));
    f.setBold(true);
    p.setFont(f);

    QRectF textRect(c.x() - r * 0.45, c.y() - r * 0.42, r * 0.90, r * 0.76);

    // Outer glow
    for (int i = 3; i >= 1; --i) {
        p.setPen(QColor(0, 180, 255, 18 * i));
        p.drawText(textRect.adjusted(-i*2, -i*2, i*2, i*2), Qt::AlignCenter, text);
    }

    // Main digit — slightly off-white for a real premium display look
    p.setPen(QColor(210, 235, 255));
    p.drawText(textRect, Qt::AlignCenter, text);

    p.restore();

    // Small "G" label above the number, minimal
    QFont labelF;
    labelF.setPixelSize(static_cast<int>(r * 0.115));
    labelF.setLetterSpacing(QFont::AbsoluteSpacing, 3);
    p.setFont(labelF);
    p.setPen(QColor(60, 100, 140, 160));
    p.drawText(QRectF(c.x() - r * 0.3, c.y() - r * 0.78, r * 0.6, r * 0.22),
               Qt::AlignCenter, "G E A R");
}
