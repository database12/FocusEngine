QT += core gui widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

TARGET   = FocusDrivingSimulator
TEMPLATE = app

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    focusengine.cpp \
    engineaudio.cpp \
    speedgaugewidget.cpp \
    gearwidget.cpp \
    mapwidget.cpp \
    rpmbarwidget.cpp \
    sessionrecord.cpp \
    sessionhistory.cpp \
    historydialog.cpp \
    etawidget.cpp \
    appwhitelist.cpp \
    allowedappsdialog.cpp \
    dashboardwidget.cpp

HEADERS += \
    mainwindow.h \
    focusengine.h \
    engineaudio.h \
    speedgaugewidget.h \
    gearwidget.h \
    mapwidget.h \
    rpmbarwidget.h \
    sessionrecord.h \
    sessionhistory.h \
    historydialog.h \
    etawidget.h \
    appwhitelist.h \
    allowedappsdialog.h \
    dashboardwidget.h

win32 {
    LIBS += -luser32 -lwinmm -ldwmapi
    RC_ICONS = app.ico
}

qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
