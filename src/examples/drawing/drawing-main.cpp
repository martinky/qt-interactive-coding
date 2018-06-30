#include <QApplication>
#include <QFileSystemWatcher>
#include <QThread>
#include <QWidget>
#include <QLabel>
#include <qicruntime.h>

#define BASE_DIR "C:/projects/qt-interactive-coding/"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    //
    // Configure the build environment.
    //
    qicRuntime rt;
    rt.setQtLibs({ "core", "gui", "widgets" });
#ifdef QT_DEBUG
    // It is extremely impmortant to ensure that the runtime-compiled code
    // links with the same version of Qt libraries and CRT library as the host
    // application (i.e. QtCore5.dll vs. QtCore5d.dll).
    rt.setQtConfig({ "debug" });
#endif
    rt.setIncludePath({ BASE_DIR "src/qicruntime",
                        BASE_DIR "src/examples/drawing" });

    //
    // We are going to watch this file and recompile and execute it whenever
    // it changes.
    //
    const QString watched = BASE_DIR "src/examples/drawing/drawing-script.cpp";

    //
    // This is our whole GUI application.
    //
    QLabel label;
    label.setMinimumSize(400, 300);
    label.setAlignment(Qt::AlignCenter);
    label.setWordWrap(true);
    label.setText("Go ahead, modify and save the watched file:\n" + watched);
    label.show();

    //
    // Make this widget available to the runtime-compiled code.
    //
    rt.setCtxVar(&label, "label", nullptr);

    QFileSystemWatcher watcher;
    watcher.addPath(watched);

    QObject::connect(&watcher, &QFileSystemWatcher::fileChanged, [&](){
        qDebug("Change detected, going to recompile: %s", qPrintable(watched));

        // Small workaround, because some editors save files in a "weird" way
        // that actually deletes and replaces the file and this confuses the
        // QFileSystemWatcher, which stops watching for subsequent changes.
        QThread::msleep(100);
        watcher.addPath(watched);

        rt.execFile(watched);
    });

    return app.exec();
}
