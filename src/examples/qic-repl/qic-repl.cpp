#include <QTextStream>
#include "qicruntime.h"

int main(int argc, const char *argv[])
{
    qicRuntime rt;
    //rt.setDebug(true);
    //rt.loadEnv("C:/projects/qt-interactive-coding/env3.txt");
    //rt.setQtLibs({ "core" });
    rt.setIncludePath({ "C:/projects/qt-interactive-coding/src/qicruntime" });

    int x = 961;
    rt.addVar(&x, "x", nullptr);

    QTextStream out(stdout);
    QTextStream in(stdin);

    out << "REPL: Type C++ code here, type 'go' to compile, or 'quit' to exit." << endl;

    QString boilerplate = "#include <qicentry.h>\n"
                          "#include <qiccontext.h>\n"
                          "extern \"C\" QIC_DLL_EXPORT void qic_entry(qicContext *ctx) {\n"
                          "    %CODE%\n"
                          "}\n";
    QString code;

    //
    // REPL
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
