#pragma once
#include <QString>
#include <QDateTime>
#include <QJsonObject>

struct SessionRecord {
    // ── Identity ───────────────────────────────────────────────
    QDateTime startTime;
    QDateTime endTime;

    // ── Time ──────────────────────────────────────────────────
    int elapsedSeconds  {0};   // actual focused time
    int goalSeconds     {0};   // session target

    // ── Performance ───────────────────────────────────────────
    double maxSpeed     {0};   // km/h
    double maxRpm       {0};   // RPM
    int    peakGear     {1};   // highest gear reached
    double avgSpeed     {0};   // km/h average over session
    double completionPct{0};   // elapsedSeconds / goalSeconds * 100

    // ── Quality ───────────────────────────────────────────────
    int    distractions {0};
    int    distractionSeconds{0};  // total seconds spent distracted
    bool   earlyFinish  {false};  // true if DONE before goal

    // ── Outcome ───────────────────────────────────────────────
    // "completed", "early_done", "cancelled", "auto_complete"
    QString outcome;

    // ── User annotation ───────────────────────────────────────
    QString taskName;   // editable label, e.g. "Implement login feature"

    // ── Serialisation ─────────────────────────────────────────
    QJsonObject toJson() const;
    static SessionRecord fromJson(const QJsonObject &obj);
};
