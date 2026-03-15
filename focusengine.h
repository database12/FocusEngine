#pragma once

#include <QObject>
#include <QTimer>
#include <QDateTime>
#include <cmath>
#include <random>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

class EngineAudioEngine;

// ─────────────────────────────────────────────────────────────────────────────
// RPM model — buffer between focus quality and speed
//
// Conceptual analogy:
//   Focus time    → ideal cruising speed (momentum curve)
//   RPM           → "engagement" / throttle state
//   Speed         → actual velocity, driven by RPM
//
// Normal operation (relaxed or strict + focused):
//   RPM climbs toward TARGET_RPM (gear-dependent cruise RPM)
//   Speed follows RPM via gear ratio
//
// Distraction (strict mode, app loses focus):
//   RPM drops at RPM_DROP_RATE per second
//   Speed is unaffected while RPM > RPM_IDLE_THRESHOLD
//   When RPM falls below threshold → speed starts bleeding off
//   On return: RPM recovers, then speed recovers
//
// RPM ranges (display: ×1000):
//   0 – 2000   idle / coasting
//   2000 – 5000 cruise band
//   5000 – 6500 power band
//   6500 – 8000 redline (only touched briefly on gear shift)
// ─────────────────────────────────────────────────────────────────────────────

class FocusEngine : public QObject
{
    Q_OBJECT

public:
    enum class State { Idle, Running, Paused };

    explicit FocusEngine(QObject *parent = nullptr);
    ~FocusEngine() override;

    bool eventFilter(QObject *watched, QEvent *event) override;

    double speed()    const { return m_speed; }
    double rpm()      const { return m_rpm; }
    int    gear()     const { return m_gear; }
    double progress() const { return m_progress; }
    State  state()    const { return m_state; }

    int focusTimeSeconds()       const { return m_focusTimeSeconds; }
    int totalDistractionsCount() const { return m_totalDistractions; }
    int totalDistractionSeconds() const { return m_totalDistractionSeconds; }

    void setSessionGoal(int s) { m_sessionGoalSeconds = s; }
    int  sessionGoal()   const { return m_sessionGoalSeconds; }
    double maxSpeed()    const { return m_maxSpeed; }
    double maxRpm()      const { return m_maxRpm; }
    int    peakGear()    const { return m_peakGear; }
    double avgSpeed()    const { return m_totalSpeedSamples > 0 ? m_speedAccum / m_totalSpeedSamples : 0.0; }
    QDateTime startTime()const { return m_startTime; }


    void setAudioMuted(bool m);
    bool audioMuted() const;

public slots:
    void start();
    void pause();
    void reset();

signals:
    void speedChanged(double speed);
    void rpmChanged(double rpm);          // NEW — drives RpmBarWidget
    void gearChanged(int gear);
    void progressChanged(double progress);
    void gearShiftTriggered(int newGear);
    void stateChanged(FocusEngine::State state);
    void sessionCompleted();
    void distractionDetected(QString reason);
    void focusResumed();

private slots:
    void onTick();

private:
    // Compute where RPM "wants" to be given current focus state
    double targetRpm()   const;
    // Compute where speed "wants" to be given current RPM
    double targetSpeed() const;

    void applySpeedClamp();
    void updateGear();
    void checkDistraction();
    bool isAppForeground() const;
    double sampleRoadVariation();

    // ── State ─────────────────────────────────────────────────────
    State  m_state{State::Idle};
    int    m_focusTimeSeconds{0};
    double m_speed{0.0};
    double m_prevSpeed{0.0};
    double m_rpm{0.0};          // current engine RPM (0 – 8000)
    int    m_gear{1};
    double m_progress{0.0};
    int    m_sessionGoalSeconds{10500}; // 7 × 25min pomodoro (8 waypoints)

    QTimer *m_tickTimer{nullptr};

    // ── Distraction state (strict mode) ───────────────────────────
    bool   m_appWasForeground{true};
    bool   m_isDistracted{false};   // true while app is out of focus
    int    m_totalDistractions{0};
    double m_maxSpeed{0};
    double m_maxRpm{0};
    int    m_peakGear{1};
    double m_speedAccum{0};
    int    m_totalSpeedSamples{0};
    QDateTime m_startTime;
    int    m_totalDistractionSeconds{0};  // cumulative seconds spent distracted

    // ── RPM model constants ───────────────────────────────────────
    // Cruise RPM target per gear (realistic mid-range cruise)
    static constexpr double CRUISE_RPM[6]  = {2200, 2800, 3200, 3500, 3800, 4000};
    // How fast RPM falls when distracted (per second)
    static constexpr double RPM_DROP_RATE  = 300.0;
    // How fast RPM recovers when focus returns (per second)
    static constexpr double RPM_RISE_RATE  = 2000.0;
    // Below this RPM, speed starts to bleed off
    static constexpr double RPM_IDLE_THRESHOLD = 800.0;
    // Maximum RPM (redline)
    static constexpr double RPM_MAX        = 8000.0;

    // ── Road variation ────────────────────────────────────────────
    std::mt19937                     m_rng;
    std::normal_distribution<double> m_noiseDist{0.0, 1.0};
    double m_smoothedNoise{0.0};
    double m_trafficOffset{0.0};
    double m_trafficTarget{0.0};
    int    m_trafficCountdown{0};

    // ── Audio ─────────────────────────────────────────────────────
    EngineAudioEngine *m_audio{nullptr};
};
