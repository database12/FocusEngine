#include "sessionhistory.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QCoreApplication>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QMap>
#include <algorithm>

SessionHistory &SessionHistory::instance()
{
    static SessionHistory inst;
    return inst;
}

SessionHistory::SessionHistory(QObject *parent) : QObject(parent)
{
    load();
}

QString SessionHistory::configPath()
{
    // Save next to the .exe for easy access / manual editing
    QString exeDir = QCoreApplication::applicationDirPath();
    return exeDir + "/session_history.json";
}

void SessionHistory::append(const SessionRecord &r)
{
    m_records.append(r);
    save();
}

void SessionHistory::load()
{
    QFile f(configPath());
    if (!f.open(QIODevice::ReadOnly)) return;
    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isObject()) return;
    for (const auto &v : doc.object()["sessions"].toArray())
        m_records.append(SessionRecord::fromJson(v.toObject()));
}

void SessionHistory::save() const
{
    QFile f(configPath());
    if (!f.open(QIODevice::WriteOnly)) return;
    QJsonArray arr;
    for (const auto &r : m_records) arr.append(r.toJson());
    QJsonObject root; root["sessions"] = arr;
    f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
}

QVector<SessionRecord> SessionHistory::forDate(const QDate &d) const
{
    QVector<SessionRecord> out;
    for (const auto &r : m_records)
        if (r.startTime.date() == d) out.append(r);
    std::sort(out.begin(), out.end(),
        [](const SessionRecord &a, const SessionRecord &b){
            return a.startTime < b.startTime; });
    return out;
}

QVector<SessionRecord> SessionHistory::forDateRange(const QDate &from, const QDate &to) const
{
    QVector<SessionRecord> out;
    for (const auto &r : m_records) {
        QDate d = r.startTime.date();
        if (d >= from && d <= to) out.append(r);
    }
    return out;
}

QVector<QDate> SessionHistory::activeDates() const
{
    QMap<QDate,bool> seen;
    for (const auto &r : m_records) seen[r.startTime.date()] = true;
    return QVector<QDate>(seen.keyBegin(), seen.keyEnd());
}

QMap<QDate,int> SessionHistory::dailyMinutes(const QDate &from, const QDate &to) const
{
    QMap<QDate,int> m;
    for (QDate d = from; d <= to; d = d.addDays(1)) m[d] = 0;
    for (const auto &r : m_records) {
        QDate d = r.startTime.date();
        if (d >= from && d <= to)
            m[d] += r.elapsedSeconds / 60;
    }
    return m;
}

bool SessionHistory::updateTaskName(const QDateTime &startTime, const QString &name)
{
    for (auto &r : m_records) {
        if (r.startTime == startTime) {
            r.taskName = name;
            save();
            return true;
        }
    }
    return false;
}
