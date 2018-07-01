#include <QTextStream>
#include <qicruntime.h>
#include <qiccontext.h>

#define BASE_DIR "C:/projects/qt-interactive-coding/"

int main()
{
    //
    // Configure the build environment.
    //
    qicRuntime rt;
    //rt.setQtLibs({ "core" });
#ifdef QT_DEBUG
    // It is extremely impmortant to ensure that the runtime-compiled code
    // links with the same version of Qt libraries and CRT library as the host
    // application (i.e. QtCore5.dll vs. QtCore5d.dll).
    rt.setQtConfig({ "debug" });
#endif
    rt.setIncludePath({ BASE_DIR "src/qicruntime" });
    //rt.loadEnv(BASE_DIR "env3.txt");

    // Add some context variables.
    int x = 961;
    rt.ctx()->set(&x, "x");

    QString boilerplate = "#include <qicentry.h>\n"
                          "#include <qiccontext.h>\n"
                          "extern \"C\" QIC_DLL_EXPORT void qic_entry(qicContext *ctx) {\n"
                          "    %CODE%\n"
                          "}\n";
    QString code;

    QTextStream out(stdout);
    QTextStream in(stdin);

    out << "REPL: Type C++ code here, then type 'go' to compile and run, or 'quit' to exit." << endl;

    //
    // REPL - Well, not exactly a REPL, rather a Read-Compile-Execute-Loop.
    //
    while (true) {
        QString line = in.readLine();
        if (line == "quit") {
            break;
        } else if (line == "clear") {
            code.clear();
        } else if (line == "go") {
            QString source = boilerplate;
            source.replace("%CODE%", code);
            rt.exec(source);
            code.clear();
        } else {
            code += line + "\n";
        }
    }

    return 0;
}
