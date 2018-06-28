TEMPLATE = app

QT += core

CONFIG += console

SOURCES += \
    qic-repl.cpp

# library: qiccontext
win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../../qicruntime/release/ -lqicruntime
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../../qicruntime/debug/ -lqicruntime
else:unix: LIBS += -L$$OUT_PWD/../../qicruntime/ -lqicruntime

INCLUDEPATH += $$PWD/../../qicruntime
DEPENDPATH += $$PWD/../../qicruntime
