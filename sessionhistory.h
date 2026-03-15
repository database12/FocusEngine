#pragma once
#include "sessionrecord.h"
#include <QObject>
#include <QVector>
#include <QDate>

class SessionHistory : public QObject
{
    Q_OBJECT
public:
    static SessionHistory &instance();

    void append(const SessionRecord &r);
    void load();
    void save() const;

    // Query
    QVector<SessionRecord> forDate(const QDate &d) const;
    QVector<SessionRecord> forDateRange(const QDate &from, const QDate &to) const;
    QVector<QDate>         activeDates() const;   // dates that have ≥1 record

    // Weekly / monthly aggregates — returns total focused minutes per day
    // keys = date, value = total focused minutes
    QMap<QDate,int> dailyMinutes(const QDate &from, const QDate &to) const;

    // Edit task name by record identity (startTime)
    bool updateTaskName(const QDateTime &startTime, const QString &name);

    static QString configPath();

private:
    explicit SessionHistory(QObject *parent = nullptr);
    QVector<SessionRecord> m_records;
};
