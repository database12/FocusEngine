#include "mapwidget.h"
#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QRadialGradient>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ─────────────────────────────────────────────────────────────────────────────
MapWidget::MapWidget(QWidget *parent) : QWidget(parent)
{
    // Route corners (t values based on 8 nodes, linear distance):
    // HOME → NORTH TURN → EAST AVE → CROSSROAD → OVERPASS → RIDGE → TUNNEL EXIT → DESTINATION
    m_waypoints = {
                   {0.000, "HOME"},
                   {0.143, "NORTH TURN"},
                   {0.286, "EAST AVE"},
                   {0.429, "CROSSROAD"},
                   {0.571, "OVERPASS"},
                   {0.714, "RIDGE"},
                   {0.857, "TUNNEL"},
                   {1.000, "DESTINATION"},
                   };

    buildStreets();
    buildRoute();

    m_carAnim = new QPropertyAnimation(this, "carOffset", this);
    m_carAnim->setDuration(800);
    m_carAnim->setEasingCurve(QEasingCurve::OutCubic);

    // Continuous pulse for the car dot (0→1→0→... )
    m_pulseAnim = new QPropertyAnimation(this, "pulsePhase", this);
    m_pulseAnim->setDuration(1400);
    m_pulseAnim->setStartValue(0.0);
    m_pulseAnim->setEndValue(1.0);
    m_pulseAnim->setLoopCount(-1);
    m_pulseAnim->start();
}

// ─────────────────────────────────────────────────────────────────────────────
// Street grid — mimics a real city block layout
// We define streets in normalised coords [0,1]×[0,1] then map to widget rect
// ─────────────────────────────────────────────────────────────────────────────
void MapWidget::buildStreets()
{
    m_streets.clear();

    // Major roads (horizontal)
    for (double y : {0.22, 0.50, 0.72}) {
        m_streets.append({{0, y}, {1, y}, true});
    }
    // Major roads (vertical)
    for (double x : {0.18, 0.42, 0.65, 0.85}) {
        m_streets.append({{x, 0}, {x, 1}, true});
    }
    // Minor roads (horizontal)
    for (double y : {0.12, 0.34, 0.60, 0.82, 0.92}) {
        m_streets.append({{0, y}, {1, y}, false});
    }
    // Minor roads (vertical)
    for (double x : {0.08, 0.28, 0.55, 0.75, 0.93}) {
        m_streets.append({{x, 0}, {x, 1}, false});
    }
    // Diagonal / arterial roads
    m_streets.append({{0.0, 0.0}, {0.42, 0.50}, false});
    m_streets.append({{0.42, 0.50}, {1.0, 0.72}, false});
    m_streets.append({{0.0, 1.0}, {0.65, 0.50}, false});
    m_streets.append({{0.65, 0.50}, {1.0, 0.22}, false});
}

// Route polyline — travels across the city with several turns
void MapWidget::buildRoute()
{
    m_route.clear();
    // Key nodes at t=0,0.25,0.50,0.75,1.0 with intentional turns
    QVector<QPointF> nodes = {
        {0.08, 0.72},   // t=0   HOME (bottom-left)
        {0.08, 0.22},   // goes north up the left arterial
        {0.42, 0.22},   // turn east on top road
        {0.42, 0.50},   // jog south to intersection
        {0.65, 0.50},   // continue east
        {0.65, 0.22},   // north again
        {0.85, 0.22},   // east to near destination
        {0.85, 0.50},   // south
        {0.93, 0.50},   // t=1   DESTINATION
    };

    // Build route nodes with linearly distributed t values
    int N = nodes.size();
    for (int i = 0; i < N; ++i) {
        double t = static_cast<double>(i) / (N - 1);
        m_route.append({nodes[i], t});
    }
}

// Interpolate along route polyline for a given t [0,1]
QPointF MapWidget::routeAt(double t) const
{
    if (m_route.isEmpty()) return {};
    t = std::clamp(t, 0.0, 1.0);
    if (t <= m_route.front().t) return m_route.front().pos;
    if (t >= m_route.back().t)  return m_route.back().pos;

    for (int i = 0; i + 1 < m_route.size(); ++i) {
        if (t >= m_route[i].t && t <= m_route[i+1].t) {
            double span = m_route[i+1].t - m_route[i].t;
            double f    = (span > 1e-9) ? (t - m_route[i].t) / span : 0.0;
            QPointF a   = m_route[i].pos;
            QPointF b   = m_route[i+1].pos;
            return a + f * (b - a);
        }
    }
    return m_route.back().pos;
}

double MapWidget::routeAngleAt(double t) const
{
    QPointF a = routeAt(t - 0.01);
    QPointF b = routeAt(t + 0.01);
    double dx = b.x() - a.x();
    double dy = b.y() - a.y();
    return std::atan2(dy, dx) * 180.0 / M_PI;
}

// ─────────────────────────────────────────────────────────────────────────────
void MapWidget::updateProgress(double progress)
{
    m_progress = progress;
    m_carAnim->stop();
    m_carAnim->setStartValue(m_carOffset);
    m_carAnim->setEndValue(progress);
    m_carAnim->start();
}

void MapWidget::updateSpeed(double speed)
{
    m_speed = speed;
    update();
}

// ─────────────────────────────────────────────────────────────────────────────
// Convert normalised [0,1] coord to widget pixel coord
// Map rect is inset from widget rect
// ─────────────────────────────────────────────────────────────────────────────
static QPointF toPixel(QPointF norm, const QRectF &r)
{
    return { r.left() + norm.x() * r.width(),
            r.top()  + norm.y() * r.height() };
}

// ─────────────────────────────────────────────────────────────────────────────
void MapWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);

    drawMapBackground(p);
    drawStreets      (p);
    drawRoute        (p);
    drawWaypointDots (p);
    drawCarArrow     (p);
    drawVignette     (p);
}

// ─────────────────────────────────────────────────────────────────────────────
void MapWidget::drawMapBackground(QPainter &p)
{
    QRectF r(0, 0, width(), height());

    // Very dark navy — like a premium GPS map at night
    p.fillRect(r, QColor(8, 14, 24));

    // Subtle block fill — block shapes between streets
    // We just darken slightly with a pattern feel
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(10, 17, 28));

    // Draw a few "block" rectangles for urban density feel
    const QRectF &wr = r;
    auto block = [&](double x1, double y1, double x2, double y2) {
        QPointF a(wr.left() + x1 * wr.width(), wr.top() + y1 * wr.height());
        QPointF b(wr.left() + x2 * wr.width(), wr.top() + y2 * wr.height());
        p.drawRect(QRectF(a, b));
    };
    // Fill between major streets with slightly lighter blocks
    p.setBrush(QColor(11, 20, 32));
    block(0.18, 0.22, 0.42, 0.50);
    block(0.42, 0.22, 0.65, 0.50);
    block(0.65, 0.22, 0.85, 0.50);
    block(0.18, 0.50, 0.42, 0.72);
    block(0.42, 0.50, 0.65, 0.72);
    block(0.65, 0.50, 0.85, 0.72);
    p.setBrush(QColor(9, 16, 26));
    block(0.0,  0.0,  0.18, 0.22);
    block(0.85, 0.0,  1.0,  0.22);
    block(0.0,  0.72, 0.18, 1.0);
    block(0.85, 0.72, 1.0,  1.0);
}

void MapWidget::drawStreets(QPainter &p)
{
    const QRectF r(0, 0, width(), height());

    for (const auto &s : m_streets) {
        QPointF a(r.left() + s.a.x() * r.width(),  r.top() + s.a.y() * r.height());
        QPointF b(r.left() + s.b.x() * r.width(),  r.top() + s.b.y() * r.height());

        if (s.major) {
            // Major road — wider, slightly brighter
            p.setPen(QPen(QColor(28, 45, 65), 2.5));
            p.drawLine(a, b);
            p.setPen(QPen(QColor(38, 60, 85, 120), 0.8));
            p.drawLine(a, b);
        } else {
            // Minor road — thin, very dim
            p.setPen(QPen(QColor(18, 30, 46), 1.2));
            p.drawLine(a, b);
        }
    }
}

void MapWidget::drawRoute(QPainter &p)
{
    const QRectF r(0, 0, width(), height());
    if (m_route.size() < 2) return;

    // Build full route path
    QPainterPath fullPath;
    for (int i = 0; i < m_route.size(); ++i) {
        QPointF pt = toPixel(m_route[i].pos, r);
        (i == 0) ? fullPath.moveTo(pt) : fullPath.lineTo(pt);
    }

    // ── Un-driven portion: dim cyan ───────────────────────────────
    p.setPen(QPen(QColor(0, 140, 160, 60), 3.5));
    p.setBrush(Qt::NoBrush);
    p.drawPath(fullPath);

    // ── Driven portion: bright glowing cyan ───────────────────────
    // Build driven sub-path up to car position
    QPainterPath drivenPath;
    bool started = false;
    for (int i = 0; i < m_route.size(); ++i) {
        if (m_route[i].t > m_carOffset + 0.001) break;
        QPointF pt = toPixel(m_route[i].pos, r);
        if (!started) { drivenPath.moveTo(pt); started = true; }
        else          drivenPath.lineTo(pt);
    }
    // Add interpolated end point exactly at car position
    if (started && m_carOffset > 0.0) {
        QPointF carNorm = routeAt(m_carOffset);
        drivenPath.lineTo(toPixel(carNorm, r));
    }

    if (!drivenPath.isEmpty()) {
        // Outer glow
        p.setPen(QPen(QColor(0, 220, 255, 35), 12));
        p.setBrush(Qt::NoBrush);
        p.drawPath(drivenPath);

        // Mid glow
        p.setPen(QPen(QColor(0, 220, 255, 70), 6));
        p.drawPath(drivenPath);

        // Core line — bright cyan
        p.setPen(QPen(QColor(0, 230, 255), 3.0));
        p.drawPath(drivenPath);
    }
}

void MapWidget::drawWaypointDots(QPainter &p)
{
    const QRectF r(0, 0, width(), height());

    for (const auto &wp : m_waypoints) {
        QPointF normPos = routeAt(wp.t);
        QPointF pos     = toPixel(normPos, r);
        bool reached    = (m_carOffset >= wp.t - 0.01);

        // Outer pulse ring (only on next unvisited waypoint)
        bool isNext = (!reached && m_carOffset >= wp.t - 0.30 && m_carOffset < wp.t);
        if (isNext) {
            double pulse = 0.5 + 0.5 * std::sin(m_pulsePhase * 2.0 * M_PI);
            QRadialGradient ring(pos, 18 + pulse * 6);
            ring.setColorAt(0,   QColor(220, 30, 100, static_cast<int>(40 * pulse)));
            ring.setColorAt(0.5, QColor(220, 30, 100, static_cast<int>(70 * pulse)));
            ring.setColorAt(1,   QColor(220, 30, 100, 0));
            p.setBrush(ring);
            p.setPen(Qt::NoPen);
            p.drawEllipse(pos, 18 + pulse * 6, 18 + pulse * 6);

            // Ring outline
            p.setPen(QPen(QColor(220, 30, 100, static_cast<int>(160 * pulse)), 1.5));
            p.setBrush(Qt::NoBrush);
            p.drawEllipse(pos, 12, 12);
        }

        if (reached) {
            // Reached: small solid cyan dot
            QRadialGradient halo(pos, 10);
            halo.setColorAt(0, QColor(0, 220, 255, 80));
            halo.setColorAt(1, QColor(0, 220, 255, 0));
            p.setBrush(halo); p.setPen(Qt::NoPen);
            p.drawEllipse(pos, 10, 10);

            p.setBrush(QColor(0, 210, 240));
            p.setPen(QPen(QColor(120, 240, 255), 1.2));
            p.drawEllipse(pos, 4, 4);
        } else {
            // Not yet reached: glowing pink/magenta dot
            QRadialGradient halo(pos, 14);
            halo.setColorAt(0, QColor(220, 40, 110, 90));
            halo.setColorAt(1, QColor(220, 40, 110, 0));
            p.setBrush(halo); p.setPen(Qt::NoPen);
            p.drawEllipse(pos, 14, 14);

            // White centre + pink ring
            p.setBrush(QColor(255, 100, 160));
            p.setPen(QPen(QColor(255, 60, 130), 1.5));
            p.drawEllipse(pos, 5.5, 5.5);
            p.setBrush(QColor(255, 240, 248));
            p.setPen(Qt::NoPen);
            p.drawEllipse(pos, 2.5, 2.5);
        }

        // Label — skip HOME once car has left (arrow is sitting right on it)
        if (wp.t == 0.0 && m_carOffset < 0.04) {
            // Still at home — draw label to the right of the arrow, not below
            QFont f("Arial", 1); f.setPixelSize(8);
            f.setLetterSpacing(QFont::AbsoluteSpacing, 1.5); f.setBold(true);
            p.setFont(f);
            p.setPen(QColor(0, 200, 230, 180));
            p.drawText(QRectF(pos.x() + 18, pos.y() - 6, 60, 12),
                       Qt::AlignLeft, wp.label);
        } else if (wp.t > 0.0) {
            QFont f("Arial", 1); f.setPixelSize(8);
            f.setLetterSpacing(QFont::AbsoluteSpacing, 1.5); f.setBold(true);
            p.setFont(f);
            QColor lc = reached ? QColor(0, 200, 230, 180) : QColor(220, 80, 140, 180);
            p.setPen(lc);
            // Place label above or below based on route position
            double ly = (normPos.y() > 0.5) ? pos.y() - 18 : pos.y() + 14;
            p.drawText(QRectF(pos.x() - 45, ly, 90, 12), Qt::AlignCenter, wp.label);
        }
        // If t==0.0 and car has moved away, don't draw HOME label at all
    }
}

void MapWidget::drawCarArrow(QPainter &p)
{
    const QRectF r(0, 0, width(), height());
    QPointF normPos = routeAt(m_carOffset);
    QPointF pos     = toPixel(normPos, r);
    double  angleDeg = routeAngleAt(m_carOffset);

    p.save();
    p.translate(pos);
    p.rotate(angleDeg + 90.0);   // +90 because arrow points "up" in local space

    // Outer glow halo
    double pulse = 0.6 + 0.4 * std::sin(m_pulsePhase * 2.0 * M_PI);
    QRadialGradient halo(QPointF(0,0), 22);
    halo.setColorAt(0, QColor(0, 190, 255, static_cast<int>(60 * pulse)));
    halo.setColorAt(1, QColor(0, 100, 200, 0));
    p.setBrush(halo); p.setPen(Qt::NoPen);
    p.drawEllipse(QPointF(0,0), 22, 22);

    // Navigation arrow — like reference image 3 & 4
    // Solid cyan/blue upward-pointing chevron
    QPainterPath arrow;
    arrow.moveTo( 0,  -16);   // tip
    arrow.lineTo( 8,    4);   // right base
    arrow.lineTo( 3,    1);   // inner right
    arrow.lineTo( 0,   10);   // tail
    arrow.lineTo(-3,    1);   // inner left
    arrow.lineTo(-8,    4);   // left base
    arrow.closeSubpath();

    // Gradient: bright tip → darker tail
    QLinearGradient grad(QPointF(0,-16), QPointF(0,10));
    grad.setColorAt(0.0, QColor(120, 230, 255));
    grad.setColorAt(0.4, QColor(  0, 185, 255));
    grad.setColorAt(1.0, QColor(  0,  80, 180));
    p.setBrush(grad);
    p.setPen(QPen(QColor(180, 240, 255, 200), 1.0));
    p.drawPath(arrow);

    // Bright tip highlight
    p.setBrush(QColor(200, 245, 255, 180));
    p.setPen(Qt::NoPen);
    QPainterPath tip;
    tip.moveTo(0, -16);
    tip.lineTo(4, -8);
    tip.lineTo(-4, -8);
    tip.closeSubpath();
    p.drawPath(tip);

    p.restore();
}


void MapWidget::drawVignette(QPainter &p)
{
    // Dark radial vignette around edges — like a real dashboard display
    QRectF r(0, 0, width(), height());
    QRadialGradient vig(r.center(), std::max(r.width(), r.height()) * 0.55);
    vig.setColorAt(0.5, QColor(0, 0, 0, 0));
    vig.setColorAt(1.0, QColor(0, 0, 0, 140));
    p.setBrush(vig);
    p.setPen(Qt::NoPen);
    p.drawRect(r);

    // Thin border
    p.setBrush(Qt::NoBrush);
    p.setPen(QPen(QColor(0, 80, 130, 50), 1));
    p.drawRoundedRect(r.adjusted(0.5, 0.5, -0.5, -0.5), 4, 4);
}
