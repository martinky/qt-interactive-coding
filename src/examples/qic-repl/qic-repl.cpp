#include <QTextStream>
#include <qicruntime.h>

int main(int argc, char *argv[])
{
    qicRuntime rt;
    //
    // NOTE: Configure the build environment according to your system here.
    //
    //rt.setQtLibs({ "core" });
    rt.setQtConfig({ /*"debug",*/ "exceptions_off", "rtti_off" });
    //rt.loadEnv("C:/projects/qt-interactive-coding/env3.txt");
    rt.setIncludePath({ "C:/projects/qt-interactive-coding/src/qicruntime" });

    // add some context variables
    int x = 961;
    rt.setCtxVar(&x, "x", nullptr);

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
    // REPL - well, not exactly a REPL, rather a Read-Compile-Execute-Loop
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
