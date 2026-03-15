#include "appwhitelist.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

AppWhitelistManager &AppWhitelistManager::instance()
{
    static AppWhitelistManager inst;
    return inst;
}

AppWhitelistManager::AppWhitelistManager(QObject *parent) : QObject(parent)
{
    // Default apps — sensible starting whitelist
    m_apps = {
        "devenv.exe",           // Visual Studio
        "code.exe",             // VS Code
        "notepad++.exe",        // Notepad++
        "notepad.exe",          // Notepad
        "sublime_text.exe",     // Sublime Text
        "idea64.exe",           // IntelliJ IDEA
        "clion64.exe",          // CLion
        "pycharm64.exe",        // PyCharm
        "cursor.exe",           // Cursor
        "wordpad.exe",          // WordPad
        "winword.exe",          // Microsoft Word
        "chrome.exe",           // Chrome (research)
        "firefox.exe",          // Firefox
        "obsidian.exe",         // Obsidian
        "FocusDrivingSimulator.exe", // this app itself
    };
    load();
}

bool AppWhitelistManager::contains(const QString &exeName) const
{
    QString lower = exeName.toLower();
    for (const auto &a : m_apps)
        if (a.toLower() == lower) return true;
    return false;
}

void AppWhitelistManager::addApp(const QString &exeName)
{
    QString norm = exeName.trimmed();
    if (norm.isEmpty() || contains(norm)) return;
    m_apps.append(norm);
    save();
}

void AppWhitelistManager::removeApp(const QString &exeName)
{
    QString lower = exeName.toLower();
    m_apps.removeIf([&](const QString &a){ return a.toLower() == lower; });
    save();
}

void AppWhitelistManager::setApps(const QStringList &list)
{
    m_apps = list;
    save();
}

QString AppWhitelistManager::configPath()
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + "/allowed_apps.json";
}

void AppWhitelistManager::load()
{
    QFile f(configPath());
    if (!f.open(QIODevice::ReadOnly)) return;

    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    if (!doc.isObject()) return;

    QJsonArray arr = doc.object().value("apps").toArray();
    if (arr.isEmpty()) return;

    m_apps.clear();
    for (const auto &v : arr)
        if (v.isString()) m_apps.append(v.toString());
}

void AppWhitelistManager::save() const
{
    QFile f(configPath());
    if (!f.open(QIODevice::WriteOnly)) return;

    QJsonArray arr;
    for (const auto &a : m_apps) arr.append(a);

    QJsonObject obj;
    obj["apps"] = arr;
    f.write(QJsonDocument(obj).toJson(QJsonDocument::Indented));
}
