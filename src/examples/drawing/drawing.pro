TEMPLATE = app

QT += core gui widgets

CONFIG += console

SOURCES += \
    drawing-main.cpp

OTHER_FILES += \
    drawing-script.cpp

# library: qiccontext
win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../../qicruntime/release/ -lqicruntime
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../../qicruntime/debug/ -lqicruntime
else:unix: LIBS += -L$$OUT_PWD/../../qicruntime/ -lqicruntime

INCLUDEPATH += $$PWD/../../qicruntime
DEPENDPATH += $$PWD/../../qicruntime
