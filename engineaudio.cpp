#include "engineaudio.h"
#include <algorithm>
#include <cstring>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ─────────────────────────────────────────────────────────────────────────────
AudioThread::AudioThread(QObject *parent) : QThread(parent) {}

AudioThread::~AudioThread()
{
    stopThread();
    wait(3000);
}

// ─────────────────────────────────────────────────────────────────────────────
// fillBuffer — petrol inline-4 synthesis
// ─────────────────────────────────────────────────────────────────────────────
void AudioThread::fillBuffer(int16_t *buf, int nSamples)
{
    const double freqTarget = m_freq.load();
    const double volTarget  = m_vol .load();
    const int    gear       = std::clamp(m_gear.load(), 1, 6);

    const double freqA = 6.0  / SR;
    const double volA  = 4.0  / SR;

    const double gF    = std::clamp((gear - 1) / 5.0, 0.0, 1.0);
    const double wSub  = 0.22 - gF * 0.09;   // f/2  piston thump
    const double wFund = 0.48;                // f    main note
    const double w2nd  = 0.23;                // 2f   warmth
    const double w3rd  = 0.13 - gF * 0.06;   // 3f   rasp/growl
    const double wNoise= 0.04;
    const double wTot  = wSub + wFund + w2nd + w3rd + wNoise;

    for (int i = 0; i < nSamples; ++i) {
        m_curFreq += (freqTarget - m_curFreq) * freqA;
        m_curVol  += (volTarget  - m_curVol)  * volA;

        double s = wFund * std::sin(m_phase)
                 + w2nd  * std::sin(m_phase * 2.0)
                 + wSub  * std::sin(m_phase * 0.5)
                 + w3rd  * std::sin(m_phase * 3.0);

        // Mechanical texture
        m_seed ^= m_seed << 13;
        m_seed ^= m_seed >> 17;
        m_seed ^= m_seed << 5;
        double raw = (static_cast<double>(m_seed & 0xFFFF) / 32768.0) - 1.0;
        m_lpNoise  = m_lpNoise * 0.93 + raw * 0.07;
        s += wNoise * m_lpNoise;

        s /= wTot;
        s = std::tanh(s * 1.25) / std::tanh(1.25);

        // Phase advance — never reset
        m_phase += TWO_PI * m_curFreq / SR;
        if (m_phase > TWO_PI * 500.0) m_phase -= TWO_PI * 500.0;

        buf[i] = static_cast<int16_t>(
            std::clamp(s * m_curVol * 32767.0, -32767.0, 32767.0));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// run() — WinMM triple-buffer realtime loop
// ─────────────────────────────────────────────────────────────────────────────
void AudioThread::run()
{
#ifdef Q_OS_WIN
    WAVEFORMATEX wfx{};
    wfx.wFormatTag      = WAVE_FORMAT_PCM;
    wfx.nChannels       = 1;
    wfx.nSamplesPerSec  = SR;
    wfx.wBitsPerSample  = 16;
    wfx.nBlockAlign     = 2;
    wfx.nAvgBytesPerSec = SR * 2;
    wfx.cbSize          = 0;

    MMRESULT res = waveOutOpen(&m_hWave, WAVE_MAPPER, &wfx, 0, 0, CALLBACK_NULL);
    if (res != MMSYSERR_NOERROR) return;

    for (int b = 0; b < N_BUFS; ++b) {
        memset(&m_headers[b], 0, sizeof(WAVEHDR));
        m_headers[b].lpData         = reinterpret_cast<LPSTR>(m_buffers[b]);
        m_headers[b].dwBufferLength = BUF_SAMP * sizeof(int16_t);
        waveOutPrepareHeader(m_hWave, &m_headers[b], sizeof(WAVEHDR));
    }

    // Pre-fill and queue all three buffers
    for (int b = 0; b < N_BUFS; ++b) {
        fillBuffer(m_buffers[b], BUF_SAMP);
        waveOutWrite(m_hWave, &m_headers[b], sizeof(WAVEHDR));
    }

    // Realtime loop: refill any completed buffer immediately
    while (!m_stop.load()) {
        bool queued = false;
        for (int b = 0; b < N_BUFS; ++b) {
            if (m_headers[b].dwFlags & WHDR_DONE) {
                m_headers[b].dwFlags &= ~WHDR_DONE;
                fillBuffer(m_buffers[b], BUF_SAMP);
                waveOutWrite(m_hWave, &m_headers[b], sizeof(WAVEHDR));
                queued = true;
            }
        }
        if (!queued) Sleep(2);
    }

    waveOutReset(m_hWave);
    for (int b = 0; b < N_BUFS; ++b)
        waveOutUnprepareHeader(m_hWave, &m_headers[b], sizeof(WAVEHDR));
    waveOutClose(m_hWave);
    m_hWave = nullptr;

#else
    // macOS / Linux stub (extend with CoreAudio / ALSA if needed)
    while (!m_stop.load()) msleep(50);
#endif
}

// ─────────────────────────────────────────────────────────────────────────────
EngineAudioEngine::EngineAudioEngine(QObject *parent) : QObject(parent)
{
    m_thread = new AudioThread(this);
}

EngineAudioEngine::~EngineAudioEngine() { stop(); }

void EngineAudioEngine::start()
{
    m_running = true;
    // Always create a fresh thread — old thread has m_stop=true after stop()
    // and QThread cannot be restarted once finished.
    if (m_thread) {
        m_thread->stopThread();
        m_thread->wait(1000);
        m_thread->deleteLater();
    }
    m_thread = new AudioThread(this);
    m_thread->setTargetFreq(speedToFreq(0.0, 1));
    m_thread->setTargetVol(m_muted ? 0.0 : 0.30);
    m_thread->start(QThread::TimeCriticalPriority);
}

void EngineAudioEngine::stop()
{
    if (!m_running) return;
    m_running = false;
    m_thread->setTargetVol(0.0);
    m_thread->stopThread();
    m_thread->wait(1500);
    // Thread stays alive; start() will replace it on next resume
}

void EngineAudioEngine::setMuted(bool m)
{
    m_muted = m;
    m_thread->setTargetVol(m ? 0.0 : m_lastVol);
}

void EngineAudioEngine::update(double speed, int gear, bool acc, bool dec)
{
    if (!m_running) return;
    m_lastVol = computeVol(speed, acc, dec);
    m_thread->setTargetFreq(speedToFreq(speed, gear));
    m_thread->setTargetGear(gear);
    if (!m_muted) m_thread->setTargetVol(m_lastVol);
}

double EngineAudioEngine::speedToFreq(double speed, int gear) const
{
    static constexpr double ratios[6] = {3.5, 2.2, 1.5, 1.1, 0.85, 0.70};
    int    g   = std::clamp(gear, 1, 6) - 1;
    double rpm = 800.0 + speed * ratios[g] * 22.0;
    return std::clamp((rpm / 60.0) * 2.0, 27.0, 234.0);
}

double EngineAudioEngine::computeVol(double speed, bool acc, bool dec) const
{
    double v = 0.30 + (speed / 180.0) * 0.55;
    if (acc) v = std::min(0.92, v * 1.12);
    if (dec) v *= 0.82;
    return std::clamp(v, 0.28, 0.92);
}
