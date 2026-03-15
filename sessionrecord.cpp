#include "sessionrecord.h"
#include <QJsonObject>

QJsonObject SessionRecord::toJson() const
{
    QJsonObject o;
    o["start"]          = startTime.toString(Qt::ISODate);
    o["end"]            = endTime.toString(Qt::ISODate);
    o["elapsed_sec"]    = elapsedSeconds;
    o["goal_sec"]       = goalSeconds;
    o["max_speed"]      = maxSpeed;
    o["max_rpm"]        = maxRpm;
    o["peak_gear"]      = peakGear;
    o["avg_speed"]      = avgSpeed;
    o["completion_pct"] = completionPct;
    o["distractions"]   = distractions;
    o["distraction_sec"] = distractionSeconds;
    o["early_finish"]   = earlyFinish;
    o["outcome"]        = outcome;
    o["task_name"]      = taskName;
    return o;
}

SessionRecord SessionRecord::fromJson(const QJsonObject &o)
{
    SessionRecord r;
    r.startTime      = QDateTime::fromString(o["start"].toString(), Qt::ISODate);
    r.endTime        = QDateTime::fromString(o["end"].toString(), Qt::ISODate);
    r.elapsedSeconds = o["elapsed_sec"].toInt();
    r.goalSeconds    = o["goal_sec"].toInt();
    r.maxSpeed       = o["max_speed"].toDouble();
    r.maxRpm         = o["max_rpm"].toDouble();
    r.peakGear       = o["peak_gear"].toInt(1);
    r.avgSpeed       = o["avg_speed"].toDouble();
    r.completionPct  = o["completion_pct"].toDouble();
    r.distractions   = o["distractions"].toInt();
    r.distractionSeconds = o["distraction_sec"].toInt();
    r.earlyFinish    = o["early_finish"].toBool();
    r.outcome        = o["outcome"].toString();
    r.taskName       = o["task_name"].toString();
    return r;
}
