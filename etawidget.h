#pragma once
#include <QWidget>

// EtaWidget — paints a compact HUD-style countdown display showing:
//   • Elapsed time (small, top)
//   • Countdown to destination / session end (large, centre)
//   • Animated progress dots
class EtaWidget : public QWidget
{
    Q_OBJECT
public:
    explicit EtaWidget(QWidget *parent = nullptr);

    void setTimes(int elapsedSecs, int remainSecs);

    QSize sizeHint()        const override { return {210, 52}; }
    QSize minimumSizeHint() const override { return {160, 42}; }

protected:
    void paintEvent(QPaintEvent *) override;

private:
    int m_elapsed{0};
    int m_remain {0};
};
