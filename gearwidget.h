#pragma once

#include <QWidget>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>

class GearWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(double flashAlpha READ flashAlpha WRITE setFlashAlpha)
    Q_PROPERTY(double scaleAnim  READ scaleAnim  WRITE setScaleAnim)

public:
    explicit GearWidget(QWidget *parent = nullptr);

    double flashAlpha() const { return m_flashAlpha; }
    void   setFlashAlpha(double v) { m_flashAlpha = v; update(); }

    double scaleAnim() const { return m_scaleAnim; }
    void   setScaleAnim(double v) { m_scaleAnim = v; update(); }

public slots:
    void updateGear(int gear);
    void triggerShiftAnimation();

protected:
    void paintEvent(QPaintEvent *) override;
    QSize sizeHint()        const override { return {280, 280}; }
    QSize minimumSizeHint() const override { return {180, 180}; }

private:
    void drawBackground(QPainter &p, QPointF c, double r);
    void drawGearDots  (QPainter &p, QPointF c, double r);
    void drawGearNumber(QPainter &p, QPointF c, double r);
    void drawTimerArc  (QPainter &p, QPointF c, double r);

    int    m_gear{1};
    double m_flashAlpha{0.0};
    double m_scaleAnim{1.0};

    QPropertyAnimation        *m_flashAnim{nullptr};
    QPropertyAnimation        *m_scaleAnimObj{nullptr};
    QParallelAnimationGroup   *m_group{nullptr};
};
