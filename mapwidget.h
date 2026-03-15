#pragma once
#include <QWidget>
#include <QPropertyAnimation>
#include <QTimer>
#include <QVector>

class MapWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(double carOffset READ carOffset WRITE setCarOffset)
    Q_PROPERTY(double pulsePhase READ pulsePhase WRITE setPulsePhase)

public:
    explicit MapWidget(QWidget *parent = nullptr);

    double carOffset()  const { return m_carOffset; }
    void   setCarOffset(double v) { m_carOffset = v; update(); }

    double pulsePhase() const { return m_pulsePhase; }
    void   setPulsePhase(double v) { m_pulsePhase = v; update(); }

public slots:
    void updateProgress(double progress);
    void updateSpeed   (double speed);

protected:
    void paintEvent(QPaintEvent *) override;
    QSize sizeHint()        const override { return {700, 190}; }
    QSize minimumSizeHint() const override { return {400, 140}; }

private:
    // ── Street grid ──────────────────────────────────────────────
    struct Street { QPointF a, b; bool major; };
    void buildStreets();

    // ── Route (polyline through waypoints) ───────────────────────
    struct RouteNode { QPointF pos; double t; };  // t = 0..1 progress
    void buildRoute();
    QPointF routeAt(double t) const;          // interpolate along route
    double  routeAngleAt(double t) const;     // heading for arrow

    void drawMapBackground(QPainter &p);
    void drawStreets      (QPainter &p);
    void drawRoute        (QPainter &p);
    void drawCarArrow     (QPainter &p);
    void drawWaypointDots (QPainter &p);
    void drawVignette     (QPainter &p);

    double m_progress {0.0};
    double m_speed    {0.0};
    double m_carOffset{0.0};   // animated, lags m_progress
    double m_pulsePhase{0.0};  // 0→1 cycle for dot pulse

    QPropertyAnimation *m_carAnim  {nullptr};
    QPropertyAnimation *m_pulseAnim{nullptr};

    QVector<Street>    m_streets;
    QVector<RouteNode> m_route;

    // Waypoint names shown at key positions
    struct WP { double t; QString label; };
    QVector<WP> m_waypoints;
};
