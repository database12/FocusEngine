#pragma once
#include <QObject>
#include <QThread>
#include <atomic>
#include <cmath>
#include <cstdint>

#ifdef Q_OS_WIN
#include <windows.h>
#include <mmsystem.h>
#endif

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ─────────────────────────────────────────────────────────────────────────────
// AudioThread — dedicated realtime thread driving WinMM waveOut directly.
// No Qt multimedia, no QAudioSink, no QSoundEffect.
// Works on every Windows machine with standard audio drivers.
//
// Triple-buffer strategy:
//   3 × 20ms PCM buffers queued into waveOut.
//   When one completes (WHDR_DONE), refill & requeue immediately.
//   Result: ~60ms total latency, zero gaps, zero clicks.
// ─────────────────────────────────────────────────────────────────────────────
class AudioThread : public QThread
{
    Q_OBJECT
public:
    explicit AudioThread(QObject *parent = nullptr);
    ~AudioThread() override;

    void setTargetFreq(double hz) { m_freq.store(hz); }
    void setTargetVol (double v)  { m_vol .store(v);  }
    void setTargetGear(int g)     { m_gear.store(g);  }
    void stopThread()             { m_stop.store(true); }

protected:
    void run() override;

private:
    void fillBuffer(int16_t *buf, int nSamples);

    std::atomic<double> m_freq{28.0};
    std::atomic<double> m_vol {0.0};
    std::atomic<int>    m_gear{1};
    std::atomic<bool>   m_stop{false};

    double   m_phase  {0.0};
    double   m_curFreq{28.0};
    double   m_curVol {0.0};
    double   m_lpNoise{0.0};
    uint32_t m_seed   {0xDEADBEEFu};

    static constexpr int    SR      = 44100;
    static constexpr double TWO_PI  = 2.0 * M_PI;
    static constexpr int    BUF_MS  = 20;
    static constexpr int    BUF_SAMP= SR * BUF_MS / 1000;   // 882 samples
    static constexpr int    N_BUFS  = 3;

#ifdef Q_OS_WIN
    HWAVEOUT m_hWave{nullptr};
    WAVEHDR  m_headers[N_BUFS]{};
    int16_t  m_buffers[N_BUFS][BUF_SAMP]{};
#endif
};

// ─────────────────────────────────────────────────────────────────────────────
class EngineAudioEngine : public QObject
{
    Q_OBJECT
public:
    explicit EngineAudioEngine(QObject *parent = nullptr);
    ~EngineAudioEngine() override;

    void start();
    void stop();
    void update(double speed, int gear, bool acc, bool dec);
    void playGearShift(int) {}
    void setMuted(bool m);
    bool isMuted() const { return m_muted; }

private:
    double speedToFreq(double speed, int gear) const;
    double computeVol (double speed, bool acc, bool dec) const;

    AudioThread *m_thread {nullptr};
    bool         m_muted  {false};
    bool         m_running{false};
    double       m_lastVol{0.30};
};
