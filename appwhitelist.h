#pragma once
#include <QObject>
#include <QStringList>

// AppWhitelistManager — singleton managing the list of allowed executable names.
// If the focused window's process is in this list, it counts as "focused".
// Persists to: <AppData>/FocusDrivingSimulator/allowed_apps.json

class AppWhitelistManager : public QObject
{
    Q_OBJECT
public:
    static AppWhitelistManager &instance();

    QStringList apps() const { return m_apps; }
    bool        contains(const QString &exeName) const;   // case-insensitive

    void addApp   (const QString &exeName);
    void removeApp(const QString &exeName);
    void setApps  (const QStringList &list);

    void load();
    void save() const;

    static QString configPath();

private:
    explicit AppWhitelistManager(QObject *parent = nullptr);
    QStringList m_apps;
};
