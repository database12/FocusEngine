#include "focusengine.h"
#include "appwhitelist.h"
#include "engineaudio.h"

#include <QApplication>
#include <QWidget>
#include <QEvent>
#include <QDateTime>
#include <algorithm>

constexpr double FocusEngine::CRUISE_RPM[6];

FocusEngine::FocusEngine(QObject *parent)
    : QObject(parent)
    , m_rng(std::random_device{}())
{
    m_audio = new EngineAudioEngine(this);
    m_tickTimer = new QTimer(this);
    m_tickTimer->setInterval(1000);
    connect(m_tickTimer, &QTimer::timeout, this, &FocusEngine::onTick);
    qApp->installEventFilter(this);
}

FocusEngine::~FocusEngine()
{
    if (qApp) qApp->removeEventFilter(this);
}

bool FocusEngine::eventFilter(QObject *watched, QEvent *event)
{
    Q_UNUSED(watched) Q_UNUSED(event)
    return false;
}

// ─────────────────────────────────────────────────────────────────────────────
// Road variation (unchanged)
// ─────────────────────────────────────────────────────────────────────────────
double FocusEngine::sampleRoadVariation()
{
    double raw = m_noiseDist(m_rng);
    m_smoothedNoise = m_smoothedNoise * 0.82 + raw * 0.18;
    // Micro noise: max ±1.5 km/h at low speed, ±3.0 at highway
    double micro = m_smoothedNoise * (1.5 + (m_speed / 180.0) * 1.5);

    if (--m_trafficCountdown <= 0) {
        std::uniform_real_distribution<double> ud(0.0, 1.0);
        double roll = ud(m_rng);
        if (m_trafficTarget < -1.0) {
            m_trafficTarget    = 0.0;
            m_trafficCountdown = 20 + static_cast<int>(ud(m_rng) * 50);
        } else if (roll < 0.20) {   // reduced from 25% to 20%
            // Traffic drag: max -5 km/h (reduced from -8)
            m_trafficTarget    = -(2.0 + ud(m_rng) * 3.0);
            m_trafficCountdown = 10 + static_cast<int>(ud(m_rng) * 15);
        } else {
            m_trafficTarget    = 0.0;
            m_trafficCountdown = 40 + static_cast<int>(ud(m_rng) * 60);
        }
    }
    m_trafficOffset += (m_trafficTarget - m_trafficOffset) * 0.06;
    return micro + m_trafficOffset;
}

// ─────────────────────────────────────────────────────────────────────────────
// RPM model
// ─────────────────────────────────────────────────────────────────────────────
double FocusEngine::targetRpm() const
{
    if (m_state != State::Running) return 900.0;
    if (m_isDistracted) return 900.0;   // idle when distracted

    // RPM based on IDEAL speed (what we're driving toward), not current speed.
    // This way RPM reflects "engine intent" rather than lagging behind speed.
    double idealSpeed = 180.0 * (1.0 - std::exp(
        -static_cast<double>(m_focusTimeSeconds) / 1200.0));

    static constexpr double ratios[6] = {3.5, 2.2, 1.5, 1.1, 0.85, 0.70};
    int g = std::clamp(m_gear, 1, 6) - 1;
    double rpm = 900.0 + idealSpeed * ratios[g] * 22.0;
    return std::clamp(rpm, 900.0, RPM_MAX);
}

double FocusEngine::targetSpeed() const
{
    double idealSpeed = 180.0 * (1.0 - std::exp(
        -static_cast<double>(m_focusTimeSeconds) / 1200.0));

    // When distracted: RPM bleeds toward idle (900).
    // Speed only starts dropping when RPM < RPM_IDLE_THRESHOLD (800).
    // At idle: speed target = 15% of ideal (coasting).
    if (m_rpm >= RPM_IDLE_THRESHOLD) {
        return idealSpeed;   // fully powered — RPM is fine
    } else {
        // Coasting: linear blend from 15% (rpm=0) to 100% (rpm=threshold)
        double t = m_rpm / RPM_IDLE_THRESHOLD;
        return idealSpeed * (0.15 + 0.85 * t);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void FocusEngine::onTick()
{
    m_prevSpeed = m_speed;

    // ── Distraction check FIRST — determines all behaviour this tick ──
    checkDistraction();

    if (m_isDistracted) {
        m_totalDistractionSeconds++;
        // ── DISTRACTED: time frozen, speed & RPM decay toward 0 ───────
        // RPM drops toward idle
        m_rpm = std::max(0.0, m_rpm - RPM_DROP_RATE);
        emit rpmChanged(m_rpm);

        // Speed bleeds off — faster decay when fully distracted
        m_speed = std::max(0.0, m_speed - 3.0);   // -3 km/h per second
        applySpeedClamp();

        bool dec = (m_speed < m_prevSpeed - 0.5);
        m_audio->update(m_speed, m_gear, false, dec);
        emit speedChanged(m_speed);
        return;   // DO NOT increment time or progress
    }

    // ── FOCUSED: normal operation ─────────────────────────────────────
    m_focusTimeSeconds++;

    // RPM update
    double rpmTarget = targetRpm();
    if (m_rpm < rpmTarget)
        m_rpm = std::min(rpmTarget, m_rpm + RPM_RISE_RATE);
    else
        m_rpm = std::max(rpmTarget, m_rpm - RPM_DROP_RATE);
    m_rpm = std::clamp(m_rpm, 0.0, RPM_MAX);
    emit rpmChanged(m_rpm);

    // Speed update
    double spdTarget = targetSpeed();
    double alpha = (spdTarget < m_speed) ? 0.22 : 0.18;
    m_speed += (spdTarget - m_speed) * alpha;

    if (m_speed > 8.0)
        m_speed += sampleRoadVariation();

    applySpeedClamp();
    updateGear();

    bool acc = (m_speed > m_prevSpeed + 0.8);
    bool dec = (m_speed < m_prevSpeed - 0.8);
    m_audio->update(m_speed, m_gear, acc, dec);

    double newProgress = std::min(1.0,
        static_cast<double>(m_focusTimeSeconds) / m_sessionGoalSeconds);
    if (newProgress != m_progress) {
        m_progress = newProgress;
        emit progressChanged(m_progress);
    }

    // Track peak metrics
    m_maxSpeed = std::max(m_maxSpeed, m_speed);
    m_maxRpm   = std::max(m_maxRpm,   m_rpm);
    m_peakGear = std::max(m_peakGear, m_gear);
    if (m_speed > 1.0) { m_speedAccum += m_speed; m_totalSpeedSamples++; }

    emit speedChanged(m_speed);

    if (m_focusTimeSeconds >= m_sessionGoalSeconds) {
        pause();
        emit sessionCompleted();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void FocusEngine::checkDistraction()
{

    bool isFg = isAppForeground();

    if (!isFg && m_appWasForeground) {
        m_isDistracted = true;
        m_totalDistractions++;
        emit distractionDetected("App lost focus");
    } else if (!isFg) {
        m_isDistracted = true;
        emit distractionDetected("Working elsewhere");
    } else if (isFg && !m_appWasForeground) {
        m_isDistracted = false;
        emit focusResumed();
    } else {
        m_isDistracted = false;
    }

    m_appWasForeground = isFg;
}

bool FocusEngine::isAppForeground() const
{
#ifdef Q_OS_WIN
    HWND fg = GetForegroundWindow();
    if (!fg) return false;

    // 1. Check if this app itself is foreground
    for (QWidget *w : QApplication::topLevelWidgets())
        if (reinterpret_cast<HWND>(w->winId()) == fg) return true;

    // 2. Check if foreground process is in the whitelist
    DWORD pid = 0;
    GetWindowThreadProcessId(fg, &pid);
    if (pid == 0) return false;

    HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!hProc) return false;

    wchar_t path[MAX_PATH] = {};
    DWORD size = MAX_PATH;
    bool allowed = false;
    if (QueryFullProcessImageNameW(hProc, 0, path, &size)) {
        // Extract just the filename (e.g. "code.exe")
        QString fullPath = QString::fromWCharArray(path);
        QString exeName  = fullPath.section('\\', -1).toLower();
        allowed = AppWhitelistManager::instance().contains(exeName);
    }
    CloseHandle(hProc);
    return allowed;

#else
    if (QApplication::activeWindow() != nullptr) return true;
    // On non-Windows, no process enumeration — just check Qt focus
    return false;
#endif
}

void FocusEngine::applySpeedClamp()
{
    m_speed = std::max(0.0, std::min(180.0, m_speed));
}

void FocusEngine::updateGear()
{
    int newGear = std::min(6, m_focusTimeSeconds / 1500 + 1);
    if (newGear != m_gear) {
        m_gear = newGear;
        emit gearChanged(m_gear);
        emit gearShiftTriggered(m_gear);
        m_audio->playGearShift(newGear);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void FocusEngine::start()
{
    if (m_state == State::Running) return;
    m_state = State::Running;
    m_appWasForeground = isAppForeground();
    m_isDistracted     = false;
    m_trafficCountdown = 30;
    m_trafficOffset    = 0.0;
    m_trafficTarget    = 0.0;
    m_smoothedNoise    = 0.0;
    m_rpm = CRUISE_RPM[0];
    m_startTime = QDateTime::currentDateTime();
    m_maxSpeed = 0; m_maxRpm = 0; m_peakGear = 1;
    m_speedAccum = 0; m_totalSpeedSamples = 0;
    m_tickTimer->start();
    m_audio->start();
    emit rpmChanged(m_rpm);
    emit stateChanged(m_state);
}

void FocusEngine::pause()
{
    if (m_state != State::Running) return;
    m_state = State::Paused;
    m_tickTimer->stop();
    m_audio->stop();

    // Reset speed & RPM to 0 — vehicle has stopped. Time & gear preserved.
    m_speed     = 0.0;
    m_prevSpeed = 0.0;
    m_rpm       = 0.0;
    m_isDistracted    = false;
    m_trafficOffset   = 0.0;
    m_trafficTarget   = 0.0;
    m_trafficCountdown= 0;
    m_smoothedNoise   = 0.0;

    emit speedChanged(m_speed);
    emit rpmChanged(m_rpm);
    emit stateChanged(m_state);
}

void FocusEngine::reset()
{
    m_tickTimer->stop();
    m_audio->stop();
    m_state              = State::Idle;
    m_focusTimeSeconds   = 0;
    m_speed              = 0.0;
    m_prevSpeed          = 0.0;
    m_rpm                = 0.0;
    m_gear               = 1;
    m_progress           = 0.0;
    m_isDistracted       = false;
    m_totalDistractions  = 0;
    m_totalDistractionSeconds = 0;
    m_trafficOffset      = 0.0;
    m_trafficTarget      = 0.0;
    m_trafficCountdown   = 0;
    m_smoothedNoise      = 0.0;

    emit speedChanged(m_speed);
    emit rpmChanged(m_rpm);
    emit gearChanged(m_gear);
    emit progressChanged(m_progress);
    emit stateChanged(m_state);
}

void FocusEngine::setAudioMuted(bool m) { m_audio->setMuted(m); }
bool FocusEngine::audioMuted()    const  { return m_audio->isMuted(); }
