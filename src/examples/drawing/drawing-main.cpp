#include <QApplication>
#include <QFileSystemWatcher>
#include <QThread>
#include <QWidget>
#include <QLabel>
#include <qicruntime.h>
#include <qiccontext.h>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    //
    // Configure the build environment.
    // Change your base_dir as needed.
    //
    qicRuntime rt;
    QString base_dir = "C:/projects/qt-interactive-coding/";
    rt.setIncludePath({ base_dir + "src/qicruntime",
                        base_dir + "src/examples/drawing" });
    // Our script will be using these Qt libraries.
    rt.setQtLibs({ "core", "gui", "widgets" });

    //
    // We are going to watch this file and recompile and execute it whenever
    // it changes.
    //
    QString watched = base_dir + "src/examples/drawing/drawing-script.cpp";

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
    // Make the label widget available to the runtime-compiled code.
    //
    rt.ctx()->set(&label, "label");

    //
    // Watch our "script" file and recompile and execute when changed.
    //
    rt.watchExecFile(watched, false);

    return app.exec();
}
